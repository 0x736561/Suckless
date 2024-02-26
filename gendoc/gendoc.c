/*
 * gendoc.c
 *
 * Copyright (C) 2022 bzt (bztsrc@gitlab)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @brief Small script to generate documentation into a single, self-contained static HTML file
 *
 */

#define GENDOC_VERSION "1.0.0"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

/**************************************** gendoc variables ****************************************/

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

enum { T_ol, T_ul, T_li, T_grid, T_gr, T_gd, T_table, T_tr, T_th, T_td, T_p, T_b, T_i, T_u, T_s, T_sup, T_sub, T_quote, T_dl,
    T_dt, T_dd, T_a, T_ui, T_alert, T_NUM };
static char *tags[T_NUM] = { "ol", "ul", "li", "grid", "gr", "gd", "table", "tr", "th", "td", "p", "b", "i", "u", "s", "sup",
    "sub", "quote", "dl", "dt", "dd", "a", "ui", "alert box" };

char gendoc_path[PATH_MAX];
char ***gendoc_rules = NULL, **gendoc_toc = NULL, **gendoc_fwd = NULL, *gendoc_buf = NULL, *gendoc_out = NULL;
char gendoc_titleimg[PATH_MAX], gendoc_theme[PATH_MAX], gendoc_lang[19][256];
int gendoc_last = -1, gendoc_first = -1, gendoc_prev = -1;
int gendoc_numrules = 0, gendoc_numtoc = 0, gendoc_numfwd = 0, gendoc_fn = 0, gendoc_l, gendoc_h = 0, gendoc_H = 0;
int gendoc_buflen = 0, gendoc_err = 0, gendoc_chk_l[T_NUM];
void gendoc_report_error(const char *msg, ...);
void gendoc_include(char *fn);
char *gendoc_gen0[] = { "\\/\\/.*?$", "\\/\\*.*?\\*\\/", "#.*?$", NULL };
char *gendoc_gen2[] = { "[:=\\<\\>\\+\\-\\*\\/%&\\^\\|!][:=]?", NULL };
char *gendoc_gen3[] = { "[0-9][0-9bx]?[0-9\\.a-fp]*", NULL };
char *gendoc_gen4[] = { "\"", "\'", "`", NULL };
char *gendoc_gen5[] = { "[", "]", "{", "}", ",", ";", ":", NULL };
char *gendoc_gen6[] = { "char", "int", "float", "true", "false", "nil", "null", "nullptr", "none", "public", "static", "struct",
    "enum", "typedef", "from", "with", "new", "delete", "void", NULL };
char *gendoc_gen7[] = { "import", "def", "if", "then", "elseif", "else", "endif", "elif", "switch", "case", "loop", "until", "for",
    "foreach", "as", "is", "in", "or", "and", "while", "do", "break", "continue", "function", "return", "try", "catch",
    "volatile", "class", "sizeof", NULL };
char **gendoc_generic[] = { gendoc_gen0, NULL, gendoc_gen2, gendoc_gen3, gendoc_gen4, gendoc_gen5, gendoc_gen6, gendoc_gen7 };

/**************************************** helper functions ****************************************/

/**
 * Allocate memory with error handling.
 */
void *myrealloc(void *buf, int size)
{
    void *ret = realloc(buf, size);
    if(!ret) {
        fprintf(stderr, "gendoc error: unable to allocate memory\n");
        exit(1);
    }
    if(!buf) memset(ret, 0, size);
    return ret;
}

/**
 * Duplicate a string with error handling. strdup() not available in ANSI C
 */
char *strdupl(char *s)
{
    char *ret;
    int i;

    if(!s || !*s) return NULL;
    i = strlen(s);
    ret = (char*)myrealloc(NULL, i + 1);
    strcpy(ret, s);
    return ret;
}

/**
 * Get file contents into memory.
 */
char *file_get_contents(char *fn, unsigned int *size)
{
    FILE *f;
    char *ret = NULL;
    unsigned int s = 0;

    f = fopen(fn, "rb");
    if(f) {
        fseek(f, 0L, SEEK_END);
        s = (unsigned int)ftell(f);
        fseek(f, 0L, SEEK_SET);
        ret = (char*)malloc(s + 16);
        if(ret) {
            s = fread(ret, 1, s, f);
            memset(ret + s, 0, 16);
        } else
            s = 0;
        fclose(f);
    }
    if(size) *size = s;
    return ret;
}

/**
 * Trim a string and remove newlines.
 */
char *trimnl(char *s)
{
    char *ret, *d;

    if(!s || !*s) return NULL;
    ret = d = (char*)myrealloc(NULL, strlen(s) + 1);
    while(*s == ' ') s++;
    while(*s) {
        if(*s != '\r' && *s != '\n')
            *d++ = *s;
        s++;
    }
    while(d > ret && d[-1] == ' ') d--;
    *d = 0;
    if(d == ret) { free(ret); return NULL; }
    ret = (char*)myrealloc(ret, d - ret + 1);
    return ret;
}

/**
 * Same as in PHP, escape unsafe characters.
 */
char *htmlspecialchars(char *s)
{
    char *ret, *d;
    int t;

    if(!s || !*s) return NULL;
    t = strlen(s) * 6;
    ret = d = (char*)myrealloc(NULL, t + 16);
    while(*s == ' ') s++;
    while(*s && d < ret + t) {
        if(*s == '&') { strcpy(d, "&amp;"); d += 5; } else
        if(*s == '<') { strcpy(d, "&lt;"); d += 4; } else
        if(*s == '>') { strcpy(d, "&gt;"); d += 4; } else
        if(*s == '\"') { strcpy(d, "&quot;"); d += 6; } else
            *d++ = *s;
        s++;
    }
    while(d > ret && d[-1] == ' ') d--;
    *d = 0;
    if(d == ret) { free(ret); return NULL; }
    ret = (char*)myrealloc(ret, d - ret + 1);
    return ret;
}

/**
 * Same as htmlspecialchars, except it handles <hl> and <hm> tags.
 */
char *preformat(char *s)
{
    char *ret, *d;
    int t;

    if(!s || !*s) return NULL;
    t = strlen(s) * 8;
    ret = d = (char*)myrealloc(NULL, t + 16);
    while(*s == ' ') s++;
    while(*s && d < ret + t) {
        if(!memcmp(s, "<hl>", 4)) { strcpy(d, "<span class=\"hl_h\">"); d += 19; s += 3; } else
        if(!memcmp(s, "<hm>", 4)) { strcpy(d, "<span class=\"hl_h hl_b\">"); d += 24; s += 3; } else
        if(!memcmp(s, "</hl>", 5) || !memcmp(s, "</hm>", 5)) { strcpy(d, "</span>"); d += 7; s += 4;
            if(s[-1] == 'm') { if(s[1] == '\r') { s++; } if(s[1] == '\n') s++; }
        } else
        if(*s == '&') { strcpy(d, "&amp;"); d += 5; } else
        if(*s == '<') { strcpy(d, "&lt;"); d += 4; } else
        if(*s == '>') { strcpy(d, "&gt;"); d += 4; } else
        if(*s == '\"') { strcpy(d, "&quot;"); d += 6; } else
            *d++ = *s;
        s++;
    }
    while(d > ret && d[-1] == ' ') d--;
    *d = 0;
    if(d == ret) { free(ret); return NULL; }
    ret = (char*)myrealloc(ret, d - ret + 1);
    return ret;
}

/**
 * RFC-compliant base64 encoder.
 */
char *base64_encode(unsigned char *s, int l)
{
    unsigned char b64e[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *ret, *out;
    int b = 0, c = 0;

    if(l <= 0) return NULL;
    ret = out = (char*)myrealloc(NULL, (((l+2)/3)*4) + 1);
    while(l) {
        b += *s++; c++; l--;
        if(c == 3) {
            *out++ = b64e[b>>18];
            *out++ = b64e[(b>>12)&0x3f];
            *out++ = b64e[(b>>6)&0x3f];
            *out++ = b64e[b&0x3f];
            b = c = 0;
        } else
            b <<= 8;
    }
    if(c != 0) {
        b <<= 16 - (8 * c);
        *out++ = b64e[b>>18];
        *out++ = b64e[(b>>12) & 0x3f];
        if(c == 1) { *out++ = '='; *out++ = '='; }
        else { *out++ = b64e[(b>>6)&0x3f]; *out++ = '='; }
    }
    *out = 0;
    return ret;
}

/**
 * A very minimalistic, non-UTF-8 aware regexp matcher. Enough to match language keywords.
 * Returns how many bytes matched, 0 if pattern doesn't match, -1 if pattern is bad.
 * Supports:
 * $      - matches end of line
 * .*?    - skip bytes until the following pattern matches
 *
 * [abc]  - one of the listed chars
 * [^abc] - all but the listed chars
 * [a-z]  - intervals, characters from 'a' to 'z'
 * [0-9]  - numbers (\d not supported)
 * a      - an exact char
 * .      - any char
 *
 * ?      - one or zero match
 * +      - at least one match
 * *      - any number of matches
 * {n}    - exactly n matches
 * {n,}   - at least n matches
 * {n,m}  - at least n, but no more than m matches
 */
int match(char *regexp, char *str)
{
    unsigned char valid[256], *c=(unsigned char*)regexp, *s=(unsigned char *)str;
    int d, r, rmin, rmax, neg;
    if(!regexp || !regexp[0] || !str || !str[0]) return -1;
    while(*c) {
        if(*c == '(' || *c == ')') { c++; continue; }
        rmin = rmax = r = 1; neg = 0;
        memset(valid, 0, sizeof(valid));
        /* special case, non-greedy match */
        if(c[0] == '.' && c[1] == '*' && c[2] == '?') {
            c += 3; if(!*c) return -1;
            if(*c == '$') { c++; while(*s && *s != '\n') s++; }
            else { while(*s && !match((char*)c, (char*)s)) s++; }
        } else {
            /* get valid characters list */
            if(*c == '\\') { c++; valid[(unsigned int)*c] = 1; } else {
                if(*c == '[') {
                    c++; if(*c == '^') { c++; neg = 1; }
                    while(*c && *c != ']') {
                        if(*c == '\\') { c++; valid[(unsigned int)*c] = 1; }
                        if(c[1] == '-') { for(d = *c, c += 2; d <= *c; d++) valid[d] = 1; }
                        else valid[(unsigned int)(*c == '$' ? 10 : *c)] = 1;
                        c++;
                    }
                    if(!*c) return -1;
                    if(neg) { for(d = 0; d < 256; d++) valid[d] ^= 1; }
                }
                else if(*c == '.') { for(d = 0; d < 256; d++) valid[d] = 1; }
                else valid[(unsigned int)(*c == '$' ? 10 : *c)] = 1;
            }
            c++;
            /* make it case-insensitive */
            for(d = 0; d < 26; d++) {
                if(valid[d + 'a']) valid[d + 'A'] = 1; else
                if(valid[d + 'A']) valid[d + 'a'] = 1;
            }
            /* get repeat count */
            if(*c == '{') {
                c++; rmin = atoi((char*)c); rmax = 0; while(*c && *c != ',' && *c != '}') c++;
                if(*c == ',') { c++; if(*c != '}') { rmax = atoi((char*)c); while(*c && *c != '}') c++; } }
                if(*c != '}') return -1;
                c++;
            }
            else if(*c == '?') { c++; rmin = 0; rmax = 1; }
            else if(*c == '+') { c++; rmin = 1; rmax = 0; }
            else if(*c == '*') { c++; rmin = 0; rmax = 0; }
            /* do the match */
            for(r = 0; *s && valid[(unsigned int)*s] && (!rmax || r < rmax); s++, r++);
            /* allow exactly one + or - inside floating point numbers if they come right after the exponent marker */
            if(r && ((str[0] >= '0' && str[0] <= '9') || (str[0] == '-' && str[1] >= '0' && str[1] <= '9')) &&
                (*s == '+' || *s == '-') && (s[-1] == 'e' || s[-1] == 'E' || s[-1] == 'p' || s[-1] == 'P'))
                    for(s++; *s && valid[(unsigned int)*s] && (!rmax || r < rmax); s++, r++);
        }
        if((!*s && *c) || r < rmin) return 0;
    }
    return (int)((intptr_t)s - (intptr_t)str);
}

/**
 * Load image into memory, detect its mime type and dimensions.
 */
unsigned char *detect_image(char *fn, char *mime, int *w, int *h, int *l)
{
    unsigned char *buf, *b;
    unsigned int size;

    mime[0] = 0; *w = *h = *l = 0;
    buf = (unsigned char*)file_get_contents(fn, &size);
    if(!buf) {
        gendoc_report_error("unable to read image '%s'", fn);
        return NULL;
    }
    *l = size;
    if(!memcmp(buf, "GIF", 3)) {
        strcpy(mime, "gif");
        *w = (buf[7] << 8) | buf[6];
        *h = (buf[9] << 8) | buf[8];
    } else
    if(!memcmp(buf, "\x89PNG", 4) && !memcmp(buf + 12, "IHDR", 4)) {
        strcpy(mime, "png");
        *w = (buf[18] << 8) | buf[19];
        *h = (buf[22] << 8) | buf[23];
    } else
    if(buf[0] == 0xff && buf[1] == 0xd8 && buf[2] == 0xff && buf[3] == 0xe0 && !memcmp(buf + 6, "JFIF", 4)) {
        strcpy(mime, "jpeg");
        for(b = buf + 20; b < buf + size - 8; b++)
            if(b[0] == 0xff && b[1] == 0xc0) {
                *w = (b[7] << 8) | b[8];
                *h = (b[5] << 8) | b[6];
                break;
            }
    } else
    if((!memcmp(buf, "RIFF", 4) && !memcmp(buf + 8, "WEBP", 4)) || !memcmp(buf, "WEBP", 4)) {
        strcpy(mime, "webp");
        if(!memcmp(buf, "RIFF", 4)) buf += 8;
        if(!memcmp(buf + 4, "VP8 ", 4)) {
            for(b = buf + 8; b < buf + size - 8; b++)
                if(b[0] == 0x9d && b[1] == 0x01 && b[2] == 0x2a) {
                    *w = ((b[4] << 8) | b[3]) & 0x3fff;
                    *h = ((b[6] << 8) | b[5]) & 0x3fff;
                    break;
                }
        } else
        if(!memcmp(buf + 4, "VP8L", 4) && buf[12] == 0x2f) {
            *w = ((buf[14] << 8) | buf[13]) & 0x3fff;
            *h = ((buf[15] << 2) | buf[14] >> 6) & 0x3fff;
        } else
        if(!memcmp(buf + 4, "VP8X", 4)) {
            *w = ((buf[17] << 8) | buf[16]) + 1;
            *h = ((buf[20] << 8) | buf[19]) + 1;
        }
    }
    if(!mime[0] || *w < 1 || *h < 1 || *w > 2048 || *h > 2048) {
        free(buf);
        gendoc_report_error("unknown file format or oversized image '%s'", fn);
        return NULL;
    }
    return buf;
}

/**
 * Check if a string is listed in an array
 */
int in_array(char *s, char **arr)
{
    int i;

    if(!s || !*s || !arr) return 0;
    for(i = 0; arr[i]; i++)
        if(!strcmp(s, arr[i])) return 1;
    return 0;
}

/**************************************** MarkDown ****************************************/

char *md_buf = NULL, *md_out = NULL;
int md_buflen = 0;

void md_text(const char *str, ...)
{
    int len;
    __builtin_va_list args;
    __builtin_va_start(args, str);
    if(!str) return;
    len = strlen(str) + 4096;
    if(!md_buf || (int)(md_out - md_buf) + len > md_buflen) {
        md_out -= (intptr_t)md_buf;
        md_buf = (char*)myrealloc(md_buf, md_buflen + len + 65536);
        memset(md_buf + md_buflen, 0, len + 65536);
        md_buflen += len + 65536;
        md_out += (intptr_t)md_buf;
    }
    md_out += vsprintf(md_out, str, args);
}

void md_write(const char *str, int len)
{
    if(!str) return;
    if(!md_buf || (int)(md_out - md_buf) + len > md_buflen) {
        md_out -= (intptr_t)md_buf;
        md_buf = (char*)myrealloc(md_buf, md_buflen + len + 65536);
        memset(md_buf + md_buflen, 0, len + 65536);
        md_buflen += len + 65536;
        md_out += (intptr_t)md_buf;
    }
    memcpy(md_out, str, len);
    md_out += len;
}

/*
 * Originally from https://github.com/Gottox/smu (MIT licensed), but modified heavily:
 * 1. changed to output to a string instead of stdout
 * 2. removed and rewrote some functions to integrate better
 * 3. output gendoc tags instead of HTML tags
 * 4. added strike-through, superscript, subscript, table
 * 5. eager to keep newlines intact (important for error reporting), this is 99% perfect
 *
 * smu - simple markup
 * Copyright (C) <2007, 2008> Enno Boland <g s01 de>
 */
#define LENGTH(x)  sizeof(x)/sizeof(x[0])
#define ADDC(b,i)  if(i % BUFSIZ == 0) { b = (char*)myrealloc(b, (i + BUFSIZ) * sizeof(char)); } b[i]

typedef int (*Parser)(const char *, const char *, int);
typedef struct {
    char *search;
    int process;
    char *before, *after;
} Tag;

static int docomment(const char *begin, const char *end, int newblock);   /* Parser for html-comments */
static int dogtlt(const char *begin, const char *end, int newblock);      /* Parser for < and > */
static int dohtml(const char *begin, const char *end, int newblock);      /* Parser for html */
static int dolineprefix(const char *begin, const char *end, int newblock);/* Parser for line prefix tags */
static int dolink(const char *begin, const char *end, int newblock);      /* Parser for links and images */
static int dolist(const char *begin, const char *end, int newblock);      /* Parser for lists */
static int doparagraph(const char *begin, const char *end, int newblock); /* Parser for paragraphs */
static int doreplace(const char *begin, const char *end, int newblock);   /* Parser for simple replaces */
static int doshortlink(const char *begin, const char *end, int newblock); /* Parser for links and images */
static int dosurround(const char *begin, const char *end, int newblock);  /* Parser for surrounding tags */
static int dounderline(const char *begin, const char *end, int newblock); /* Parser for underline tags */
static int dotable(const char *begin, const char *end, int newblock);     /* Parser for tables */
static void md_parse(const char *begin, const char *end, int isblock);    /* Processes range between begin and end. */

/* list of parsers */
static Parser parsers[] = { dounderline, docomment, dolineprefix, dotable,
                            dolist, dosurround, doparagraph, dogtlt, dolink,
                            doshortlink, dohtml, doreplace };

static Tag lineprefix[] = {
    { ">",      2,  "<quote>",  "</quote>" },
    { "###### ",1,  "<h6>",     "</h6>" },
    { "##### ", 1,  "<h5>",     "</h5>" },
    { "#### ",  1,  "<h4>",     "</h4>" },
    { "### ",   1,  "<h3>",     "</h3>" },
    { "## ",    1,  "<h2>",     "</h2>" },
    { "# ",     1,  "<h1>",     "</h1>" }
};

static Tag underline[] = {
    { "=",      1,  "<h1>",     "</h1>" },
    { "-",      1,  "<h2>",     "</h2>" },
};

static Tag surround[] = {
    { "```",    0,  "<pre>",    "</pre>" },    /* must be the first. If language given, will become <code> */
    { "``",     0,  "<tt>",     "</tt>" },
    { "`",      0,  "<tt>",     "</tt>" },
    { "^^",     1,  "<sup>",    "</sup>" },
    { ",,",     1,  "<sub>",    "</sub>" },
    { "___",    1,  "<u><i><b>","</b></i></u>" },
    { "***",    1,  "<i><b>",   "</b></i>" },
    { "~~",     1,  "<s>",      "</s>" },
    { "__",     1,  "<u>",      "</u>" },
    { "**",     1,  "<b>",      "</b>" },
    { "~",      1,  "<s>",      "</s>" },
    { "_",      1,  "<u>",      "</u>" },
    { "*",      1,  "<i>",      "</i>" },
};

static const char *replace[][2] = {
    { "\\\\",   "\\" },
    { "\\`",    "`" },
    { "\\*",    "*" },
    { "\\_",    "_" },
    { "\\~",    "~" },
    { "\\^",    "^" },
    { "\\,",    "," },
    { "\\{",    "{" },
    { "\\}",    "}" },
    { "\\[",    "[" },
    { "\\]",    "]" },
    { "\\(",    "(" },
    { "\\)",    ")" },
    { "\\#",    "#" },
    { "\\+",    "+" },
    { "\\-",    "-" },
    { "\\.",    "." },
    { "\\!",    "!" },
};

static const char *insert[][2] = {
    { "  \n",   "<br>\n" },
};


int
dogtlt(const char *begin, const char *end, int newblock) {
    int brpos;
    char c;

    (void)newblock;
    if(begin + 1 >= end)
        return 0;
    brpos = begin[1] == '>';
    if(!brpos && *begin != '<')
        return 0;
    c = begin[brpos ? 0 : 1];
    if(!brpos && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '1' || c > '6')) {
        md_text("&lt;");
        return 1;
    }
    else if(brpos && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '1' || c > '6') && !strchr("/\"'",c)) {
        md_text("%c&gt;",c);
        return 2;
    }
    return 0;
}

int
docomment(const char *begin, const char *end, int newblock) {
    char *p;

    (void)newblock;
    if(strncmp("<!--", begin, 4))
        return 0;
    p = strstr(begin, "-->");
    if(!p || p + 3 >= end)
        return 0;
    return (p + 3 - begin) * (newblock ? -1 : 1);
}

int
dohtml(const char *begin, const char *end, int newblock) {
    const char *p, *tag, *tagend;

    (void)newblock;
    if(begin + 2 >= end)
        return 0;
    p = begin;
    if(p[0] != '<' || !((p[1] >= 'a' && p[1] <= 'z') || (p[1] >= 'A' && p[1] <= 'Z')))
        return 0;
    p++;
    tag = p;
    for(; *p != ' ' && *p != '>' && p < end; p++);
    tagend = p;
    if(p > end || tag == tagend/* || (strncmp(tag, "pre", 3) && strncmp(tag, "code", 4) && strncmp(tag, "tt", 2))*/)
        return 0;
    while((p = strstr(p, "</")) && p < end) {
        p += 2;
        if(strncmp(p, tag, tagend - tag) == 0 && p[tagend - tag] == '>') {
            p++;
            md_parse(begin, p - 3, 0);
            md_write(p - 3, tagend - tag + 3);
            return p - begin + tagend - tag;
        }
    }
    p = strchr(tagend, '>');
    if(p) {
        md_write(begin, p - begin + 1);
        return p - begin + 1;
    }
    else
        return 0;
}

int
dolineprefix(const char *begin, const char *end, int newblock) {
    unsigned int i, j, l;
    char *buffer;
    const char *p;

    if(newblock)
        p = begin;
    else if(*begin == '\n')
        p = begin + 1;
    else
        return 0;
    for(i = 0; i < LENGTH(lineprefix); i++) {
        l = strlen(lineprefix[i].search);
        if(end - p < l)
            continue;
        if(strncmp(lineprefix[i].search, p, l))
            continue;
        if(*begin == '\n')
            md_text("\n");
        md_text("%s", lineprefix[i].before);
        if(lineprefix[i].search[l-1] == '\n') {
            md_text("\n");
            return l - 1;
        }
        buffer = (char*)myrealloc(NULL, BUFSIZ);
        buffer[0] = '\0';

        /* Collect lines into buffer while they start with the prefix */
        j = 0;
        while((strncmp(lineprefix[i].search, p, l) == 0) && p + l < end) {
            p += l;

            /* Special case for blockquotes: optional space after > */
            if(lineprefix[i].search[0] == '>' && *p == ' ') {
                p++;
            }

            while(p < end) {
                ADDC(buffer, j) = *p;
                j++;
                if(*(p++) == '\n')
                    break;
            }
        }

        /* Skip empty lines in block */
/*
        while(*(buffer + j - 1) == '\n') {
            j--;
        }
*/
        ADDC(buffer, j) = '\0';
        if(lineprefix[i].process)
            md_parse(buffer, buffer + strlen(buffer), lineprefix[i].process >= 2);
        else
            md_text("%s", buffer);
        md_text("%s\n", lineprefix[i].after);
        free(buffer);
        return -(p - begin);
    }
    return 0;
}

int
dolink(const char *begin, const char *end, int newblock) {
    int img, len, parens_depth = 1;
    const char *desc, *link, *p, *q, *descend, *linkend;

    (void)newblock;
    if(*begin == '[')
        img = 0;
    else if(strncmp(begin, "![", 2) == 0)
        img = 1;
    else
        return 0;
    for(p = desc = begin + 1 + img; p < end && *p != '\n' && *p != ']'; p++);
    if(p >= end || p[1] != '(')
        return 0;
    for(q = strstr(desc, "!["); q && q < end && q < p; q = strstr(q + 1, "!["))
        if(!(p = strstr(p + 1, "](")) || p > end)
            return 0;
    descend = p;
    link = p + 2;

    /* find end of link while handling nested parens */
    q = link;
    while(parens_depth) {
        if(!(q = strpbrk(q, "()")) || q > end)
            return 0;
        if(*q == '(')
            parens_depth++;
        else
            parens_depth--;
        if(parens_depth && q < end)
            q++;
    }
    linkend = q;

    /* Links can be given in angular brackets */
    if(*link == '<' && *(linkend - 1) == '>') {
        link++;
        linkend--;
    }

    len = q + 1 - begin;
    if(img) {
        md_text("<img%c ", begin[-1] == '\n' ? 'w' : 't');
        md_write(link, linkend - link);
        md_text(">");
        if(descend > desc) {
            md_text("<fig>");
            md_write(desc, descend - desc);
            md_text("</fig>");
        }
    }
    else {
        md_text("<a ");
        md_write(link, linkend - link);
        md_text(">");
        md_parse(desc, descend, 0);
        md_text("</a>");
    }
    return len;
}

int
dolist(const char *begin, const char *end, int newblock) {
    unsigned int i, j, indent, run, ul, isblock;
    const char *p, *q;
    char *buffer = NULL;
    char marker;

    isblock = 0;
    if(newblock)
        p = begin;
    else if(*begin == '\n')
        p = begin + 1;
    else
        return 0;
    q = p;
    if(*p == '-' || *p == '*' || *p == '+') {
        ul = 1;
        marker = *p;
    } else {
        ul = 0;
        for(; p < end && *p >= '0' && *p <= '9'; p++);
        if(p >= end || *p != '.')
            return 0;
    }
    p++;
    if(p >= end || !(*p == ' ' || *p == '\t'))
        return 0;
    for(p++; p != end && (*p == ' ' || *p == '\t'); p++);
    indent = p - q;
    buffer = (char*)myrealloc(buffer, BUFSIZ);
    if(!newblock || *begin == '\n')
        md_text("\n");
    md_text(ul ? "<ul>" : "<ol>");
    run = 1;
    for(; p < end && run; p++) {
        for(i = 0; p < end && run; p++, i++) {
            if(*p == '\n') {
                if(p + 1 == end)
                    break;
                else {
                    /* Handle empty lines */
                    for(q = p + 1; (*q == ' ' || *q == '\t') && q < end; q++);
                    if(*q == '\n') {
                        ADDC(buffer, i) = '\n';
                        i++;
                        run = 0;
                        isblock++;
                        p = q;
                    }
                }
                q = p + 1;
                j = 0;
                if(ul && *q == marker)
                    j = 1;
                else if(!ul) {
                    for(; q + j != end && q[j] >= '0' && q[j] <= '9' && j < indent; j++);
                    if(q + j == end)
                        break;
                    if(j > 0 && q[j] == '.')
                        j++;
                    else
                        j = 0;
                }
                if(q + indent < end)
                    for(; (q[j] == ' ' || q[j] == '\t') && j < indent; j++);
                if(j == indent) {
                    ADDC(buffer, i) = '\n';
                    i++;
                    p += indent;
                    run = 1;
                    if(*q == ' ' || *q == '\t')
                        p++;
                    else
                        break;
                }
                else if (j < indent)
                    run = 0;
            }
            ADDC(buffer, i) = *p;
        }
        ADDC(buffer, i) = '\0';
        md_text("<li>");
        md_parse(buffer, buffer + i, isblock > 1 || (isblock == 1 && run));
        md_text("</li>\n");
    }
    if(md_out > md_buf && md_out[-1] == '\n') md_out--;
    md_text(ul ? "</ul>" : "</ol>");
    free(buffer);
    p--;
    while(*(--p) == '\n');
    return -(p - begin + 1);
}

int
doparagraph(const char *begin, const char *end, int newblock) {
    const char *p, *s, *tag = "p";

    if(!newblock)
        return 0;
    p = strstr(begin, "\n\n");
    if(!p || p > end)
        p = end;
    for(s = begin; s < p && *s != '\n'; s++);
    if(p - begin <= 1 || (s == p && *begin == '<' && p[-1] == '>'))
        return 0;
    s = begin;
    if(!memcmp(begin, "INFO:", 5)) { s += 5; tag = "info"; } else
    if(!memcmp(begin, "HINT:", 5)) { s += 5; tag = "hint"; } else
    if(!memcmp(begin, "NOTE:", 5)) { s += 5; tag = "note"; } else
    if(!memcmp(begin, "SEE ALSO:", 9)) { s += 9; tag = "also"; } else
    if(!memcmp(begin, "ALSO:", 5)) { s += 5; tag = "also"; } else
    if(!memcmp(begin, "TODO:", 5)) { s += 5; tag = "todo"; } else
    if(!memcmp(begin, "WARNING:", 8)) { s += 8; tag = "warn"; } else
    if(!memcmp(begin, "WARN:", 5)) { s += 5; tag = "warn"; }
    md_text("<%s>", tag);
    md_parse(s, p, 0);
    md_text("</%s>", tag);
    return -(p - begin);
}

int
doreplace(const char *begin, const char *end, int newblock) {
    unsigned int i, l;

    (void)newblock;
    for(i = 0; i < LENGTH(insert); i++)
        if(strncmp(insert[i][0], begin, strlen(insert[i][0])) == 0)
            md_text("%s", insert[i][1]);
    for(i = 0; i < LENGTH(replace); i++) {
        l = strlen(replace[i][0]);
        if(end - begin < l)
            continue;
        if(strncmp(replace[i][0], begin, l) == 0) {
            md_text("%s", replace[i][1]);
            return l;
        }
    }
    return 0;
}

int
doshortlink(const char *begin, const char *end, int newblock) {
    const char *p;

    (void)newblock;
    if(*begin != '[')
        return 0;
    for(p = begin + 1; p != end && *p != '\\' && *p != '\n' && *p != ']'; p++);
    if(*p == ']') {
        md_text("<a>");
        md_write(begin + 1, p - (begin + 1));
        md_text("</a>");
        return p - begin + 1;
    }
    return 0;
}

int
dosurround(const char *begin, const char *end, int newblock) {
    unsigned int i, l;
    const char *p, *start, *stop, *lang = NULL;

    (void)newblock;
    for(i = 0; i < LENGTH(surround); i++) {
        l = strlen(surround[i].search);
        if(end - begin < 2*l || strncmp(begin, surround[i].search, l) != 0)
            continue;
        start = begin + l;
        p = start - 1;
        do {
            stop = p;
            p = strstr(p + 1, surround[i].search);
        } while(p && p[-1] == '\\');
        if (p && p[-1] != '\\')
            stop = p;
        if(!stop || stop < start || stop >= end)
            continue;
        if(!i && *start != ' ' && *start != '\n')
            for(lang = start; start < stop && *start != '\n'; start++);
        if(!i && lang) {
            md_text("<code ");
            md_write(lang, start - lang);
            md_text(">");
        } else
            md_text("%s", surround[i].before);

        /* Single space at start and end are ignored */
        if (*start == ' ' && *(stop - 1) == ' ') {
            start++;
            stop--;
            l++;
        }

        if(surround[i].process)
            md_parse(start, stop, 0);
        else
            md_write(start, stop - start);
        if(!i && lang)
            md_text("</code>");
        else
            md_text("%s", surround[i].after);
        return stop - begin + l;
    }
    return 0;
}

int
dounderline(const char *begin, const char *end, int newblock) {
    unsigned int i, j, l, k;
    const char *p;

    if(!newblock)
        return 0;
    p = begin;
    for(l = k = 0; p + l != end && p[l] != '\n'; l++)
        if(p[l] > 0 || ((uint8_t)p[l] & 0xC0) == 0xC0) k++;
    p += l + 1;
    if(l == 0)
        return 0;
    for(i = 0; i < LENGTH(underline); i++) {
        for(j = 0; p + j != end && p[j] != '\n' && p[j] == underline[i].search[0]; j++);
        if(j >= k) {
            md_text("%s", underline[i].before);
            if(underline[i].process)
                md_parse(begin, begin + l, 0);
            else
                md_write(begin, l);
            md_text("%s\n", underline[i].after);
            return -(j + p - begin);
        }
    }
    return 0;
}

static char intable = 0, inrow, incell;
static long int calign;

int
dotable(const char *begin, const char *end, int newblock) {
    const char *p, cells[] = "dnDN";
    int i, l = (int)sizeof(calign) * 4;

    (void)newblock;
    if(*begin != '|')
        return 0;
    if(inrow && (begin + 1 >= end || begin[1] == '\n')) {
        md_text("</t%c></tr>", inrow == -1 ? 'h' : 'd');
        inrow = 0;
        if(begin + 2 >= end || begin[2] == '\n') {
            intable = 0;
            md_text("</table>");
        }
        return 1;
    }
    if(begin < end && (begin[1] == '-' || begin[1] == ':' || begin[1] == '*')) {
        for(p = begin; p < end && *p != '\n'; p++);
        return p - begin;
    }
    if(!intable) {
        intable = 1; inrow = -1; incell = 0; calign = 0;
        /* look ahead and get cell alignments */
        for(p = begin + 1; p < end && *p != '\n'; p++);
        for(; p < end && *p == '\n'; p++);
        if(p < end && (p[1] == '-' || p[1] == ':' || p[1] == '*'))
            for(i = -1; p < end && *p != '\n'; p++)
                if(i < l)
                    switch(*p) {
                        case '|': i++; break;
                        case ':': calign |= 1 << (i * 2); break;
                        case '*': calign |= (p[1] == '|' ? 3 : 2) << (i * 2); break;

                    }
        md_text("<table><tr>");
    }
    if(!inrow) {
        inrow = 1; incell = 0;
        md_text("<tr>");
    }
    if(incell)
        md_text("</t%c>", inrow == -1 ? 'h' : 'd');
    l = incell < l ? (calign >> (incell * 2)) & 3 : 0;
    md_text("<t%c>", inrow == -1 ? (l > 1 ? 'H' : 'h') : cells[l]);
    incell++;
    for(p = begin + 1; p < end && *p == ' '; p++);
    return p - begin;
}

/* originally was "process", renamed to match gendoc conventions */
void
md_parse(const char *begin, const char *end, int newblock) {
    const char *p, *q;
    int affected;
    unsigned int i;

    for(p = begin; p < end;) {
        if(newblock)
            while(*p == '\n') {
                md_text("\n");
                if(++p == end)
                    return;
            }
        affected = 0;
        for(i = 0; i < LENGTH(parsers) && !affected; i++)
            affected = parsers[i](p, end, newblock);
        p += abs(affected);
        if(!affected) {
            md_text("%c", *p);
            p++;
        }
        for(q = p; q != end && *q == '\n'; q++);
        if(q == end)
            return;
        else if(p[0] == '\n' && p + 1 != end && p[1] == '\n')
            newblock = 1;
        else
            newblock = affected < 0;
    }
}

/**************************************** gendoc interface ****************************************/

/**
 * Convert a string into an URL and id attribute-safe string.
 * Sometimes it sucks that internet was designed by English only speakers.
 * @param string input
 * @return string the converted string in an alloc'd buffer
 */
char *gendoc_safeid(char *str)
{
    int i, l, t;
    char *ret, *s = str, *d, *unicodes[] = {
        /* first 4 bytes: zero terminated UTF-8 character, replace from; rest of the string: replace to lowercase ASCII */
        "À\0\0a","à\0\0a","Á\0\0a","á\0\0a","Â\0\0a","â\0\0a","Ã\0\0a","ã\0\0a","Ä\0\0a","ä\0\0a","Å\0\0a","å\0\0a","Æ\0\0ae",
        "æ\0\0ae","Ç\0\0c","ç\0\0c","È\0\0e","è\0\0e","É\0\0e","é\0\0e","Ê\0\0e","ê\0\0e","Ë\0\0e","ë\0\0e","Ì\0\0i","ì\0\0i",
        "Í\0\0i","í\0\0i","Î\0\0i","î\0\0i","Ï\0\0i","ï\0\0i","Ð\0\0d","ð\0\0d","Ñ\0\0n","ñ\0\0n","Ò\0\0o","ò\0\0o","Ó\0\0o",
        "ó\0\0o","Ô\0\0o","ô\0\0o","Õ\0\0o","õ\0\0o","Ö\0\0o","ö\0\0o","Ø\0\0o","ø\0\0o","Ù\0\0u","ù\0\0u","Ú\0\0u","ú\0\0u",
        "Û\0\0u","û\0\0u","Ü\0\0u","ü\0\0u","Ý\0\0y","ý\0\0y","Þ\0\0p","þ\0\0p","Ā\0\0a","ā\0\0a","Ă\0\0a","ă\0\0a","Ą\0\0a",
        "ą\0\0a","Ć\0\0c","ć\0\0c","Ĉ\0\0c","ĉ\0\0c","Ċ\0\0c","ċ\0\0c","Č\0\0c","č\0\0c","Ď\0\0d","ď\0\0d","Đ\0\0d","đ\0\0d",
        "Ē\0\0e","ē\0\0e","Ĕ\0\0e","ĕ\0\0e","Ė\0\0e","ė\0\0e","Ę\0\0e","ę\0\0e","Ě\0\0e","ě\0\0e","Ĝ\0\0g","ĝ\0\0g","Ğ\0\0g",
        "ğ\0\0g","Ġ\0\0g","ġ\0\0g","Ģ\0\0g","ģ\0\0g","Ĥ\0\0h","ĥ\0\0h","Ħ\0\0h","ħ\0\0h","Ĩ\0\0i","ĩ\0\0i","Ī\0\0i","ī\0\0i",
        "Ĭ\0\0i","ĭ\0\0i","Į\0\0i","į\0\0i","İ\0\0i","i\0\0\0i","Ĳ\0\0ij","ĳ\0\0ij","Ĵ\0\0j","ĵ\0\0j","Ķ\0\0k","ķ\0\0k","Ĺ\0\0l",
        "ĺ\0\0l","Ļ\0\0l","ļ\0\0l","Ľ\0\0l","ľ\0\0l","Ŀ\0\0l","ŀ\0\0l","Ł\0\0l","ł\0\0l","Ń\0\0n","ń\0\0n","Ņ\0\0n","ņ\0\0n",
        "Ň\0\0n","ň\0\0n","Ŋ\0\0n","ŋ\0\0n","Ō\0\0o","ō\0\0o","Ŏ\0\0o","ŏ\0\0o","Ő\0\0o","ő\0\0o","Œ\0\0ce","œ\0\0ce","Ŕ\0\0r",
        "ŕ\0\0r","Ŗ\0\0r","ŗ\0\0r","Ř\0\0r","ř\0\0r","Ś\0\0s","ś\0\0s","Ŝ\0\0s","ŝ\0\0s","Ş\0\0s","ş\0\0s","Š\0\0s","š\0\0s",
        "Ţ\0\0t","ţ\0\0t","Ť\0\0t","ť\0\0t","Ŧ\0\0t","ŧ\0\0t","Ũ\0\0u","ũ\0\0u","Ū\0\0u","ū\0\0u","Ŭ\0\0u","ŭ\0\0u","Ů\0\0u",
        "ů\0\0u","Ű\0\0u","ű\0\0u","Ų\0\0u","ų\0\0u","Ŵ\0\0w","ŵ\0\0w","Ŷ\0\0y","ŷ\0\0y","Ÿ\0\0y","ÿ\0\0y","Ź\0\0z","ź\0\0z",
        "Ż\0\0z","ż\0\0z","Ž\0\0z","ž\0\0z","Ɓ\0\0b","ɓ\0\0b","Ƃ\0\0b","ƃ\0\0b","Ƅ\0\0b","ƅ\0\0b","Ɔ\0\0c","ɔ\0\0c","Ƈ\0\0c",
        "ƈ\0\0c","Ɖ\0\0ɖ","ɖ\0\0d","Ɗ\0\0d","ɗ\0\0d","Ƌ\0\0d","ƌ\0\0d","Ǝ\0\0e","ǝ\0\0e","Ə\0\0e","ə\0\0e","Ɛ\0\0e","ɛ\0\0e",
        "Ƒ\0\0f","ƒ\0\0f","Ɠ\0\0g","ɠ\0\0g","Ɣ\0\0y","ɣ\0\0y","Ɩ\0\0l","ɩ\0\0l","Ɨ\0\0i","ɨ\0\0i","Ƙ\0\0k","ƙ\0\0k","Ɯ\0\0w",
        "ɯ\0\0w","Ɲ\0\0n","ɲ\0\0n","Ɵ\0\0o","ɵ\0\0o","Ơ\0\0o","ơ\0\0o","Ƣ\0\0oj","ƣ\0\0oj","Ƥ\0\0p","ƥ\0\0p","Ʀ\0\0r","ʀ\0\0r",
        "Ƨ\0\0s","ƨ\0\0s","Ʃ\0\0s","ʃ\0\0s","Ƭ\0\0t","ƭ\0\0t","Ʈ\0\0t","ʈ\0\0t","Ư\0\0u","ư\0\0u","Ʊ\0\0u","ʊ\0\0u","Ʋ\0\0u",
        "ʋ\0\0u","Ƴ\0\0y","ƴ\0\0y","Ƶ\0\0z","ƶ\0\0z","Ʒ\0\0z","ʒ\0\0z","Ƹ\0\0z","ƹ\0\0z","Ƽ\0\0z","ƽ\0\0z","Ǆ\0\0dz","ǆ\0\0dz",
        "ǅ\0\0dz","Ǉ\0\0lj","ǉ\0\0lj","ǈ\0\0lj","Ǌ\0\0nj","ǌ\0\0nj","ǋ\0\0nj","Ǎ\0\0a","ǎ\0\0a","Ǐ\0\0i","ǐ\0\0i","Ǒ\0\0o","ǒ\0\0o",
        "Ǔ\0\0u","ǔ\0\0u","Ǖ\0\0u","ǖ\0\0u","Ǘ\0\0u","ǘ\0\0u","Ǚ\0\0u","ǚ\0\0u","Ǜ\0\0u","ǜ\0\0u","Ǟ\0\0a","ǟ\0\0a","Ǡ\0\0a",
        "ǡ\0\0a","Ǣ\0\0ae","ǣ\0\0ae","Ǥ\0\0g","ǥ\0\0g","Ǧ\0\0g","ǧ\0\0g","Ǩ\0\0k","ǩ\0\0k","Ǫ\0\0o","ǫ\0\0o","Ǭ\0\0o","ǭ\0\0o",
        "Ǯ\0\0z","ǯ\0\0z","Ǳ\0\0dz","ǳ\0\0dz","ǲ\0\0dz","Ǵ\0\0g","ǵ\0\0g","Ƕ\0\0hj","ƕ\0\0hj","Ƿ\0\0p","ƿ\0\0p","Ǹ\0\0n","ǹ\0\0n",
        "Ǻ\0\0a","ǻ\0\0a","Ǽ\0\0ae","ǽ\0\0ae","Ǿ\0\0o","ǿ\0\0ǿ","Ȁ\0\0a","ȁ\0\0a","Ȃ\0\0a","ȃ\0\0a","Ȅ\0\0e","ȅ\0\0e","Ȇ\0\0e",
        "ȇ\0\0e","Ȉ\0\0i","ȉ\0\0i","Ȋ\0\0i","ȋ\0\0i","Ȍ\0\0o","ȍ\0\0o","Ȏ\0\0o","ȏ\0\0o","Ȑ\0\0r","ȑ\0\0r","Ȓ\0\0r","ȓ\0\0r",
        "Ȕ\0\0u","ȕ\0\0u","Ȗ\0\0u","ȗ\0\0u","Ș\0\0s","ș\0\0s","Ț\0\0t","ț\0\0t","Ȝ\0\0z","ȝ\0\0z","Ȟ\0\0h","ȟ\0\0h","Ƞ\0\0n",
        "ƞ\0\0n","Ȣ\0\0o","ȣ\0\0o","Ȥ\0\0z","ȥ\0\0z","Ȧ\0\0a","ȧ\0\0a","Ȩ\0\0e","ȩ\0\0e","Ȫ\0\0o","ȫ\0\0o","Ȭ\0\0o","ȭ\0\0o",
        "Ȯ\0\0o","ȯ\0\0o","Ȱ\0\0o","ȱ\0\0o","Ȳ\0\0y","ȳ\0\0y","Ⱥ\0\0a","ⱥ\0a","Ȼ\0\0c","ȼ\0\0c","Ƚ\0\0l","ƚ\0\0l","Ⱦ\0\0t",
        "ⱦ\0t","Ƀ\0\0b","ƀ\0\0b","Ʉ\0\0u","ʉ\0\0u","Ɇ\0\0e","ɇ\0\0e","Ɉ\0\0j","ɉ\0\0j","Ɋ\0\0q","ɋ\0\0q","Ɍ\0\0r","ɍ\0\0r",
        "Ɏ\0\0y","ɏ\0\0y"
    };

    if(!str || !*str) return NULL;
    t = 2 * strlen(str); if(t > 252) t = 252; /* <- 255 - zero - 2 bytes for the longest UTF-8 replacement */
    ret = d = (char*)myrealloc(NULL, t + 5);
    while(*s == ' ') s++;
    while(*s && d < ret + t) {
        if(*s == '\r' || *s == '\"' || *s == '\'' || *s == '#' || *s == '?' || *s == '/' || *s == '&' || *s == ';') {
            s++; continue;
        }
        if(*s == '&')
            while(*s && *s != ';') s++;
        if((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9')) {
            if(*s >= 'A' && *s <= 'Z')
                *d++ = (*s++) + 'a' - 'A';
            else
                *d++ = *s++;
            continue;
        } else {
            for(i = 0; i < (int)(sizeof(unicodes)/sizeof(unicodes[0])); i++) {
                l = strlen(unicodes[i]);
                if(!memcmp(s, unicodes[i], l)) {
                    s += l; l = strlen(unicodes[i] + 4);
                    memcpy(d, unicodes[i] + 4, l); d += l;
                    break;
                }
            }
            if(i < (int)(sizeof(unicodes)/sizeof(unicodes[0]))) continue;
        }
        if(d > ret && d[-1] != '_') *d++ = '_';
        s++;
    }
    while(d > ret && d[-1] == '_') d--;
    *d = 0;
    if(d == ret) { free(ret); return NULL; }
    ret = (char*)myrealloc(ret, d - ret + 1);
    return ret;
}

/**
 * Check if a tag is open
 */
void gendoc_chk_o(int i)
{
    if(i == T_li && !gendoc_chk_l[T_ol] && !gendoc_chk_l[T_ul])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in an ol/ul\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if((i == T_dt || i == T_dd) && !gendoc_chk_l[T_dl])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in a dl\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_gr && !gendoc_chk_l[T_grid])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in a grid\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_gd && !gendoc_chk_l[T_gr])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in a gr\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_tr && !gendoc_chk_l[T_table])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in a table\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if((i == T_th || i == T_td) && !gendoc_chk_l[T_tr])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s not in a tr\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i < T_b)
        gendoc_chk_l[i]++;
    else {
        if(gendoc_chk_l[i]) {
            fprintf(stderr, "gendoc error: %s:%u: tag %s already open (opened in line %u)\n", gendoc_path, gendoc_l, tags[i],
                gendoc_chk_l[i]);
            gendoc_err++;
        } else
            gendoc_chk_l[i] = gendoc_l;
    }
}

/**
 * Check if a tag is closed
 */
void gendoc_chk_c(int i)
{
    if((i == T_ol || i == T_ul) && gendoc_chk_l[T_li] > gendoc_chk_l[i])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tag li still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_dl && (gendoc_chk_l[T_dt] > gendoc_chk_l[i] || gendoc_chk_l[T_dd] > gendoc_chk_l[i]))
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tags dt/dd still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_gr && gendoc_chk_l[T_gd] > gendoc_chk_l[i])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tag gd still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_gd && gendoc_chk_l[T_gr] > gendoc_chk_l[i])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tag gr still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_grid && gendoc_chk_l[T_gr] > gendoc_chk_l[i])
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tag gr still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(i == T_gr && (gendoc_chk_l[T_th] > gendoc_chk_l[i] || gendoc_chk_l[T_td] > gendoc_chk_l[i]))
        { fprintf(stderr, "gendoc error: %s:%u: tag %s close but tag th/td/tn still open\n", gendoc_path, gendoc_l, tags[i]); gendoc_err++; }
    if(!gendoc_chk_l[i]) {
        fprintf(stderr, "gendoc error: %s:%u: no opening tag %s\n", gendoc_path, gendoc_l, tags[i]);
        gendoc_err++;
    } else {
        if(i < T_b)
            gendoc_chk_l[i]--;
        else
            gendoc_chk_l[i] = 0;
    }
}

/**
 * Check if all tags are closed
 */
void gendoc_chk_all(void)
{
    int i;
    for(i = 0; i < T_NUM; i++)
        if(gendoc_chk_l[i]) {
            if(gendoc_chk_l[i] < T_b)
                fprintf(stderr, "gendoc error: %s:%u: tag %s not closed\n", gendoc_path, gendoc_l, tags[i]);
            else
                fprintf(stderr, "gendoc error: %s:%u: tag %s not closed (opened in line %u)\n", gendoc_path, gendoc_l, tags[i],
                    gendoc_chk_l[i]);
            gendoc_err++;
            gendoc_chk_l[i] = 0;
        }
}

/**
 * Report an error.
 * @param string error message
 * @param arguments
 */
void gendoc_report_error(const char *msg, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, msg);
    fprintf(stderr, "gendoc error: %s:%u: ", gendoc_path, gendoc_l);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    gendoc_err++;
}

/**
 * Add any arbitrary text to output. If a tag is open, then to that.
 * @param string the text
 * @param arguments
 */
void gendoc_text(const char *str, ...)
{
    int len;
    __builtin_va_list args;
    __builtin_va_start(args, str);
    if(!str) return;
    len = strlen(str) + 4096;
    if(!gendoc_buf || (int)(gendoc_out - gendoc_buf) + len > gendoc_buflen) {
        gendoc_out -= (intptr_t)gendoc_buf;
        gendoc_buf = (char*)myrealloc(gendoc_buf, gendoc_buflen + len + 65536);
        memset(gendoc_buf + gendoc_buflen, 0, len + 65536);
        gendoc_buflen += len + 65536;
        gendoc_out += (intptr_t)gendoc_buf;
    }
    gendoc_out += vsprintf(gendoc_out, str, args);
}

/**
 * Parse <doc> tag.
 * @param string with <doc> sub-tags
 * @param end of string
 */
void gendoc_doc(char *s, char *e)
{
    char *d, *f;

    while(s < e) {
        while(*s != '<' && s < e) s++;
        if(*s != '<' || s >= e) break;
        s++; for(d = s; *d != '<' && d < e; d++);
        if(!memcmp(s, "lang>", 5))      { s += 5; memcpy(gendoc_lang[0], s, d - s); gendoc_lang[0][d - s] = 0; } else
        if(!memcmp(s, "titleimg>", 9)) {
            s += 9; memcpy(gendoc_titleimg, gendoc_path, gendoc_fn); f = gendoc_titleimg + gendoc_fn;
            while(s < d && *s != ' ' && *s != '<') *f++ = *s++;
            *f = 0; if(*s == ' ') { s++; memcpy(gendoc_lang[1], s, d - s); gendoc_lang[1][d - s] = 0; }
        } else
        if(!memcmp(s, "title>", 6))   { s += 6; memcpy(gendoc_lang[2], s, d - s); gendoc_lang[2][d - s] = 0; } else
        if(!memcmp(s, "url>", 4))     { s += 4; memcpy(gendoc_lang[3], s, d - s); gendoc_lang[3][d - s] = 0; } else
        if(!memcmp(s, "version>", 8)) { s += 8; memcpy(gendoc_lang[4], s, d - s); gendoc_lang[4][d - s] = 0; } else
        if(!memcmp(s, "theme>", 6))   {
            s += 6;
            memcpy(gendoc_theme, gendoc_path, gendoc_fn); memcpy(gendoc_theme + gendoc_fn, s, d - s);
            gendoc_theme[gendoc_fn + (intptr_t)(d - s)] = 0;
        } else
        if(!memcmp(s, "rslt>", 5))    { s += 5; memcpy(gendoc_lang[5], s, d - s); gendoc_lang[5][d - s] = 0; } else
        if(!memcmp(s, "home>", 5))    { s += 5; memcpy(gendoc_lang[6], s, d - s); gendoc_lang[6][d - s] = 0; } else
        if(!memcmp(s, "link>", 5))    { s += 5; memcpy(gendoc_lang[7], s, d - s); gendoc_lang[7][d - s] = 0; } else
        if(!memcmp(s, "info>", 5))    { s += 5; memcpy(gendoc_lang[8], s, d - s); gendoc_lang[8][d - s] = 0; } else
        if(!memcmp(s, "hint>", 5))    { s += 5; memcpy(gendoc_lang[9], s, d - s); gendoc_lang[9][d - s] = 0; } else
        if(!memcmp(s, "note>", 5))    { s += 5; memcpy(gendoc_lang[10], s, d - s); gendoc_lang[10][d - s] = 0; } else
        if(!memcmp(s, "also>", 5))    { s += 5; memcpy(gendoc_lang[11], s, d - s); gendoc_lang[11][d - s] = 0; } else
        if(!memcmp(s, "todo>", 5))    { s += 5; memcpy(gendoc_lang[12], s, d - s); gendoc_lang[12][d - s] = 0; } else
        if(!memcmp(s, "warn>", 5))    { s += 5; memcpy(gendoc_lang[13], s, d - s); gendoc_lang[13][d - s] = 0; } else
        if(!memcmp(s, "args>", 5))    { s += 5; memcpy(gendoc_lang[14], s, d - s); gendoc_lang[14][d - s] = 0; } else
        if(!memcmp(s, "rval>", 5))    { s += 5; memcpy(gendoc_lang[15], s, d - s); gendoc_lang[15][d - s] = 0; } else
        if(!memcmp(s, "prev>", 5))    { s += 5; memcpy(gendoc_lang[16], s, d - s); gendoc_lang[16][d - s] = 0; } else
        if(!memcmp(s, "next>", 5))    { s += 5; memcpy(gendoc_lang[17], s, d - s); gendoc_lang[17][d - s] = 0; } else
        if(!memcmp(s, "copy>", 5))    { s += 5; memcpy(gendoc_lang[18], s, d - s); gendoc_lang[18][d - s] = 0; }
        s = d + 1;
    }
}

/**
 * Add link to previous page.
 */
void gendoc_prev_link()
{
    gendoc_chk_all();
    if(gendoc_prev != -1) {
        gendoc_text("<br style=\"clear:both;\"><label class=\"btn prev\" accesskey=\"p\" for=\"_");
        if(gendoc_prev != -2) {
            gendoc_text("%s\" title=\"%s", gendoc_toc[gendoc_prev * 4 + 0], gendoc_toc[gendoc_prev * 4 + 3]);
        }
        gendoc_text("\">%s</label>", gendoc_lang[16]);
    }
}

/**
 * Add link to next page.
 * @param string toc id
 */
void gendoc_next_link(int toc)
{
    gendoc_chk_all();
    if(toc >= 0 && gendoc_toc[toc * 4 + 0] && gendoc_toc[toc * 4 + 0][0]) {
        if(gendoc_prev == -1)
            gendoc_text("<br style=\"clear:both;\">");
        gendoc_text("<label class=\"btn next\" accesskey=\"n\" for=\"_%s\" title=\"%s\">%s</label>",
            gendoc_toc[toc * 4 + 0], gendoc_toc[toc * 4 + 3], gendoc_lang[17]);
    }
    gendoc_text("</div>\n");
}

/**
 * Add an internal link to any heading.
 * @param string heading user-readable name
 */
void gendoc_internal_link(char *name)
{
    int i;
    char *lnk = gendoc_safeid(name), *sub = NULL;

    if(!name || !*name || !lnk) return;
    for(i = 0; i < gendoc_numtoc; i++)
        if(gendoc_toc[i * 4 + 0] && !strcmp(gendoc_toc[i * 4 + 0], lnk)) break;
    if(i >= gendoc_numtoc) {
        for(i = 0; i < gendoc_numfwd && strcmp(gendoc_fwd[i], lnk); i++);
        if(i == gendoc_numfwd) {
            gendoc_numfwd++;
            gendoc_fwd = (char**)myrealloc(gendoc_fwd, gendoc_numfwd * sizeof(char*));
            gendoc_fwd[i] = (char*)myrealloc(NULL, strlen(lnk) + 1 + strlen(gendoc_path) + strlen(name) + 32);
            sprintf(gendoc_fwd[i], "%s%c%s:%u unresolved link: %s", lnk, 0, gendoc_path, gendoc_l, name);
        }
        sub = lnk;
        lnk = (char*)myrealloc(NULL, strlen(sub) + 10);
        strcpy(lnk, "@GENDOC:");
        strcat(lnk, sub);
        strcat(lnk, "@");
    }
    gendoc_text("<a href=\"#%s\" onclick=\"c('%s')\">%s</a>", lnk, lnk, name);
    free(lnk);
    if(sub) free(sub);
}

/**
 * Resolve a forward internal link.
 * @param string heading user-readable name
 * @param string the real id
 */
void gendoc_resolve_link(char *subst, char *id)
{
    char *sub, *lnk, *s;
    int i, j;

    if(!subst || !id || !*id || !gendoc_buf || gendoc_out < gendoc_buf + 10) return;
    sub = gendoc_safeid(subst);
    if(!sub) return;
    for(i = j = 0; i < gendoc_numfwd; i++)
        if(!strcmp(gendoc_fwd[i], sub)) {
            free(gendoc_fwd[i]);
            memcpy(&gendoc_fwd[i], &gendoc_fwd[i + 1], (gendoc_numfwd - i - 1) * sizeof(char*));
            gendoc_numfwd--; i--; j++;
        }
    if(!j) { free(sub); return; }
    lnk = (char*)myrealloc(NULL, strlen(sub) + 10);
    strcpy(lnk, "@GENDOC:");
    strcat(lnk, sub);
    strcat(lnk, "@");
    free(sub);
    i = strlen(lnk); j = strlen(id);
    for(s = gendoc_buf; s < gendoc_out - i; s++)
        if(*s == '@' && !memcmp(s, lnk, i)) {
            sub = gendoc_buf;
            gendoc_out -= (intptr_t)gendoc_buf;
            gendoc_buf = (char*)myrealloc(NULL, gendoc_buflen + j - i + 65536);
            gendoc_buflen += j - i + 65536;
            memcpy(gendoc_buf, sub, s - sub);
            memcpy(gendoc_buf + (intptr_t)(s - sub), id, j);
            memcpy(gendoc_buf + (intptr_t)(s - sub) + j, s + i, (intptr_t)gendoc_out - i - (intptr_t)(s - sub));
            gendoc_out += (intptr_t)gendoc_buf + j - i;
            s = gendoc_buf + (intptr_t)(s - sub) + i;
            free(sub);
        }
    free(lnk);
}

/**
 * Add a heading.
 * @param string from "h1>..."
 * @param string position of the ending tag
 */
void gendoc_heading(char *s, char *e)
{
    char *id = NULL, *alias = NULL, level = s[1];
    int i;

    gendoc_chk_all();
    s += 2;
    if(*s == ' ') {
        s++; id = s; while(s < e && *s != ' ' && *s != '>') s++;
        if(*s == ' ') {
            alias = id; *s++ = 0; id = s;
            while(s < e && *s != '>') s++;
        }
        if(*s == '>')
            *s++ = 0;
    } else
        s++;
    if(s >= e && gendoc_first != -2) {
        gendoc_report_error("empty heading name");
        return;
    }
    *e = 0;
    if(gendoc_h) {
        if(level == '1')
            gendoc_text("<div class=\"page\" rel=\"_\">");
        gendoc_text("\n<h%c>%s</h%c>", level, s, level);
    } else {
        gendoc_toc = (char**)myrealloc(gendoc_toc, (gendoc_numtoc + 1) * 4 * sizeof(char*));
        gendoc_toc[gendoc_numtoc * 4 + 0] = gendoc_safeid(id ? id : s);
        if(!gendoc_toc[gendoc_numtoc * 4 + 0] || !gendoc_toc[gendoc_numtoc * 4 + 0][0]) {
            gendoc_report_error("no id for heading (%s)", s);
            return;
        }
        for(i = 0; i < gendoc_numtoc; i++)
            if(gendoc_toc[i * 4 + 0] && !strcmp(gendoc_toc[i * 4 + 0], gendoc_toc[gendoc_numtoc * 4 + 0])) {
                gendoc_report_error("id for heading isn't unique (%s)", gendoc_toc[gendoc_numtoc * 4 + 0]);
                gendoc_toc[gendoc_numtoc * 4 + 0][0] = 0;
                break;
            }
        gendoc_toc[gendoc_numtoc * 4 + 1] = (char*)((intptr_t)level);
        gendoc_toc[gendoc_numtoc * 4 + 2] = strdupl(s);
        gendoc_toc[gendoc_numtoc * 4 + 3] = htmlspecialchars(s);
        gendoc_resolve_link(s, gendoc_toc[gendoc_numtoc * 4 + 0]);
        if(gendoc_out)
            while(gendoc_out > gendoc_buf && (gendoc_out[-1] == ' ' || gendoc_out[-1] == '\r' || gendoc_out[-1] == '\n'))
                gendoc_out--;
        if(level == '1') {
            if(gendoc_numtoc) {
                gendoc_prev_link();
                gendoc_next_link(gendoc_numtoc);
                gendoc_prev = gendoc_last;
                gendoc_last = gendoc_numtoc;
            }
            if(gendoc_first == -1) gendoc_first = gendoc_numtoc;
            gendoc_text("<div class=\"page\"");
            if(gendoc_toc[gendoc_numtoc * 4 + 0] && gendoc_toc[gendoc_numtoc * 4 + 0][0])
                gendoc_text(" rel=\"%s\"", gendoc_toc[gendoc_numtoc * 4 + 0]);
            gendoc_text("><div><ul class=\"breadcrumbs\"><li><label class=\"home\" for=\"_");
            if(gendoc_first >= 0) gendoc_text("%s", gendoc_toc[gendoc_first * 4 + 0]);
            gendoc_text("\" title=\"%s\"></label>&nbsp;»</li><li>&nbsp;%s</li></ul><hr></div>", gendoc_lang[6], s);
        }
        gendoc_text("\n");
        alias = gendoc_safeid(alias);
        if(alias) {
            gendoc_text("<span id=\"%s\"></span>", alias);
            free(alias);
        }
        gendoc_text("<h%c", level);
        if(gendoc_toc[gendoc_numtoc * 4 + 0] && gendoc_toc[gendoc_numtoc * 4 + 0][0])
            gendoc_text(" id=\"%s\"", gendoc_toc[gendoc_numtoc * 4 + 0]);
        gendoc_text(">%s", s);
        if(gendoc_toc[gendoc_numtoc * 4 + 0] && gendoc_toc[gendoc_numtoc * 4 + 0][0]) {
            gendoc_text("<a href=\"#%s\"></a>", gendoc_toc[gendoc_numtoc * 4 + 0]);
        }
        gendoc_text("</h%c>", level);
        gendoc_numtoc++;
    }
}

/**
 * Add source code.
 * @param string the source code
 * @param string the language type (or empty string)
 */
void gendoc_source_code(char *str, char *lang)
{
    char *s, *d, ***r = NULL, *hl = "cpons.tkfv";
    int i, j, k, l, m, nt = 0, at = 0, *t = NULL;

    /* load highlight rules */
    if(gendoc_rules && lang && *lang) {
        for(i = 0; i < gendoc_numrules; i++)
            if(!strcmp((char*)gendoc_rules[i * 9], lang)) { r = &gendoc_rules[i * 9 + 1]; break; }
    }
    /* use a generic scheme */
    if(!r) {
        if(lang && *lang)
            fprintf(stderr, "gendoc warning: %s:%u: no highlight rules for '%s' using generics\n", gendoc_path, gendoc_l, lang);
        r = gendoc_generic;
    }
    /* dump current ruleset. Note: quotes (") are deliberately unescaped. */
    /*
    for(j = 0; j < 8; j++) {
        printf("r[%d] (hl_%c): ", j, hl[j]);
        if(r[j])
            for(i = 0; r[j][i]; i++)
                printf(" \"%s\"", r[j][i]);
        printf("\n");
    }
    printf("\n\n");
    */
    gendoc_text("<div class=\"pre\"><pre class=\"lineno\">");
    if(str && *str) {
        for(; *str && (*str == '\r' || *str == '\n'); str++);
        for(s = str; *s; s++);
        for(; s > str && (s[-1] == ' ' || s[-1] == '\r' || s[-1] == '\n'); s--);
        *s = 0;
        for(i = 1, s = str; *s; s++)
            if(*s == '\n') gendoc_text("%u<br>", i++);
        gendoc_text("%u<br></pre><code>", i);
        /* tokenize string */
        for(k = 0; str[k]; ) {
            if(nt + 2 >= at) {
                t = (int*)myrealloc(t, (at + 256) * sizeof(int));
                memset(t + at, 0, 256 * sizeof(int));
                at += 256;
            }
            if(!k && str[0] == '#' && str[1] == '!') {
                t[nt++] = 0; while(str[k] && str[k] != '\r' && str[k] != '\n') k++;
                continue;
            }
            if(!memcmp(str + k, "<hl>", 4) || !memcmp(str + k, "<hm>", 4) ||
              !memcmp(str + k, "</hl>", 5) || !memcmp(str + k, "</hm>", 5)) {
                if(!nt || (t[nt - 1] & 0xf) != 5) t[nt++] = (k << 4) | 5;
                k += str[k + 1] == '/' ? (str[k + 3] == 'm' ? (str[k + 5] == '\n' ? 6 : (str[k + 5] == '\r' && str[k + 6] == '\n' ? 7 : 5)) : 5) : 4;
                continue;
            }
            if(str[k] == '(') { t[nt++] = (k << 4) | 5; k++; continue; }
            j = 0; if(r[5]) for(i = 0; r[5][i]; i++) if(r[5][i][0] == str[k]) { j = 1; break; }
            if(str[k] == ')' || str[k] == ' ' || str[k] == '\t' || str[k] == '\r' || str[k] == '\n' || j) {
                if(!nt || (t[nt - 1] & 0xf) != 5) t[nt++] = (k << 4) | 5;
                k++; continue;
            }
            for(m = 0; m < 4; m++)
                if(r[m] && !(m == 3 && nt && (t[nt - 1] & 0xf) == 9))
                    for(i = 0; r[m][i]; i++) {
                        l = match(r[m][i], str + k);
                        /* special case, only match alphabetic operators if not inside an alphabetic string */
                        if(l > 0 && (m != 2 || !((str[k] >= 'a' && str[k] <= 'z') || (str[k] >= 'A' && str[k] <= 'Z')) || (
                          ((!k || str[k - 1] == ' ' || str[k - 1] == '\t' || str[k - 1] == '\r' || str[k - 1] == '\n' || str[k - 1] == ')' ||
                            str[k - 1] == ']' || (str[k - 1] >= '0' && str[k - 1] <= '9')) &&
                          (!str[k + l] || str[k + l] == ' ' || str[k + l] == '\t' || str[k + l] == '\r' || str[k + l] == '\n' || str[k + l] == '(' ||
                            str[k + l] == '[' || (str[k + l] >= '0' && str[k + l] <= '9')))))) {
                            if(!nt || (t[nt - 1] & 0xf) != m) t[nt++] = (k << 4) | m;
                            k += l - 1; goto nextchar;
                        }
                    }
            if(r[4])
                for(i = 0; r[4][i]; i++) {
                    l = strlen(r[4][i]);
                    if(!memcmp(str + k, r[4][i], l)) {
                        if(!nt || (t[nt - 1] & 0xf) != 4) t[nt++] = (k << 4) | 4;
                        for(k += l; str[k]; k++) {
                            if(str[k] == '\\') k++; else
                            if(str[k] == r[4][i][l - 1]) { if(str[k + 1] != r[4][i][l - 1]) break; else k++; }
                        }
                        goto nextchar;
                    }
                }
            if(!nt || (t[nt - 1] & 0xf) != 9) t[nt++] = (k << 4) | 9;
nextchar:   if(str[k]) k++;
        }
        if(t) {
            for(i = 0; i < nt; i++) {
                if((t[i] & 0xf) == 9) {
                    j = i + 1 < nt ? t[i + 1] >> 4 : k;
                    l = t[i] >> 4;
                    s = d = (char*)myrealloc(NULL, j - l + 1);
                    for(; l < j; l++)
                        *d++ = str[l] >= 'A' && str[l] <= 'Z' ? str[l] + 'a' - 'A' : str[l];
                    *d = 0;
                    if(in_array(s, r[6])) t[i] = (t[i] & ~0xf) | 6; else
                    if(in_array(s, r[7])) t[i] = (t[i] & ~0xf) | 7; else
                    if(str[l] == '(' || (i + 2 < nt && (t[i + 2] & 0xf) == 5 && str[t[i + 2] >> 4] == '('))
                        t[i] = (t[i] & ~0xf) | 8;
                    free(s);
                }
                if(i && (t[i] & 0xf) == 3 && (t[i - 1] & 0xf) == 2 && (str[t[i - 1] >> 4] == '-' || str[t[i - 1] >> 4] == '.'))
                    t[i] -= 16;
            }
            /* concatenate tokens into a string with highlight spans */
            for(i = l = 0; i < nt; i++) {
                j = i + 1 < nt ? t[i + 1] >> 4 : k;
                if(j == l) continue;
                if((t[i] & 0xf) != 5) gendoc_text("<span class=\"hl_%c\">", hl[(t[i] & 0xf)]);
                for(; l < j; l++) {
                    if(!memcmp(str + l, "<hl>", 4)) { gendoc_text("<span class=\"hl_h\">"); l += 3; } else
                    if(!memcmp(str + l, "<hm>", 4)) { gendoc_text("<span class=\"hl_h hl_b\">"); l += 3; } else
                    if(!memcmp(str + l, "</hl>", 5) || !memcmp(str + l, "</hm>", 5)) { gendoc_text("</span>"); l += 4;
                        if(str[l - 1] == 'm') { if(str[l + 1] == '\r') { l++; } if(str[l + 1] == '\n') l++; }
                    } else
                    if(str[l] == '&') gendoc_text("&amp;"); else
                    if(str[l] == '<') gendoc_text("&lt;"); else
                    if(str[l] == '>') gendoc_text("&gt;"); else
                    if(str[l] == '\"') gendoc_text("&quot;"); else
                        gendoc_text("%c", str[l]);
                }
                if((t[i] & 0xf) != 5) gendoc_text("</span>");
            }
            free(t);
        }
    } else
        gendoc_text("</pre><code>");
    gendoc_text("</code></div>");
}

/**
 * Add an image.
 * @param string either "t" (inlined as text), "l" (left), "r" (right), "c" (centered) or "w" (wide, centered)
 * @param string image's file name
 */
void gendoc_image(char align, char *fn)
{
    char path[PATH_MAX], mime[8] = { 0 }, *s;
    unsigned char *buf;
    int l, w = 0, h = 0;

    l = strlen(gendoc_lang[3]);
    memcpy(path, gendoc_path, gendoc_fn);
    strcpy(path + gendoc_fn, !memcmp(fn, gendoc_lang[3], l) ? fn + l : fn);
    buf = detect_image(path, mime, &w, &h, &l);
    if(!buf) return;
    if(align != 'l' && align != 'r' && align != 'c' && align != 'w') align= 't';
    if(align == 'c') gendoc_text("<div class=\"imgc\">");
    gendoc_text("<img class=\"img%c\"", align);
    if(align == 't' && h > 22) { w = 22 * w / h; h = 22; }
    if(align != 'w')
        gendoc_text(" width=\"%u\" height=\"%u\"", w, h);
    s = strrchr(fn, '/'); if(!s) s = fn; else s++;
    s = htmlspecialchars(s);
    gendoc_text(" alt=\"%s\"", s);
    free(s);
    s = base64_encode(buf, l);
    l = strlen(s);
    free(buf);
    gendoc_text(" src=\"data:image/%s;base64,", mime);
    /* gendoc_text() assumes that arguments are no longer than 4k, that might not be true with base64 encoded images
     * luckily base64 does not use % so it is safe to use it as a formatting string without arguments */
    gendoc_text(s);
    gendoc_text("\">");
    if(align == 'c') gendoc_text("</div>");
    free(s);
}

/**
 * Generate API documentation.
 * @param string language
 * @param string file name
 */
void gendoc_api(char *lang, char *fn)
{
    char path[PATH_MAX], *s, *p, *e, *l, *h, *o, *q, t, *buf;

    if(!lang || !*lang || !fn || !*fn) return;

    memcpy(path, gendoc_path, gendoc_fn);
    strcpy(path + gendoc_fn, fn);
    buf = s = (char*)file_get_contents(path, NULL);
    if(!buf) {
        gendoc_report_error("unable to read source '%s'", path);
        return;
    }
    while(*s) {
        if(s[0] == '/' && s[1] == '*' && s[2] == '*' && s[3] != '*') {
            s += 3; for(e = s; *e && (e[-2] != '*' || e[-1] != '/'); e++);
            if(!*e) break;
            for(q = e - 2; *e && (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n'); e++);
            if(!*e) break;
            for(p = e; *e && *e != '\r' && *e != '\n'; e++);
            path[0] = *e; *e = 0;
            gendoc_text("<dl><dt>");
            gendoc_source_code(p, lang);
            gendoc_text("</dt><dd>");
            t = 0;
            while(*s && s < q) {
                for(; s < q && *s && (*s == '\r' || *s == '\n' || *s == ' ' || *s == '*'); s++);
                if(!*s) break;
                for(l = s; l < q && *l && *l != '\r' && *l != '\n'; l++);
                *l = 0; h = o = htmlspecialchars(s);
                if(h) {
                    if(!memcmp(h, "@param ", 7)) {
                        for(h += 7; *h == ' '; h++);
                        if(!t) { t = 1; gendoc_text("<div class=\"table\"><table><tr><th>%s</th></tr>", gendoc_lang[14]); }
                        gendoc_text("<tr><td>%s</td></tr>", h);
                    } else
                    if(!memcmp(h, "@return ", 8)) {
                        for(h += 8; *h == ' '; h++);
                        if(!t) { t = 1; gendoc_text("<div class=\"table\"><table>"); }
                        gendoc_text("<tr><th>%s</th></tr><tr><td>%s</td></tr>", gendoc_lang[15], h);
                    } else
                    if(!t)
                        gendoc_text("%s ", h);
                    free(o);
                }
                s = l + 1;
            }
            if(t) gendoc_text("</table></div>");
            gendoc_text("</dd></dl><br>");
            *e = path[0];
            if(!*s) break;
        }
        s++;
    }
    free(buf);
}

/**
 * Parse a source document.
 * @param string source document's content
 * @param gendoc_path name of the source file
 * @return gendoc_l current line number, must be incremented by the reader
 */
void gendoc_parse(char *str)
{
    char *s = str, *d, *e, tmp[8] = { 0 };

    while(*s) {
        /* skip comments */
        if(!memcmp(s, "<!--", 4)) {
            for(s += 4; *s && memcmp(s, "-->", 3); s++)
                if(*s == '\n') gendoc_l++;
            if(*s) s += 3;
            continue;
        }
        /* copy everything between tags */
        if(*s == '\n') gendoc_l++;
        if(*s != '<') {
            if(*s == ' ' || *s == '\t') {
                if(s == str || (s[-1] != ' ' && s[-1] != '\t' && s[-1] != '\n')) gendoc_text(" ");
                s++; continue;
            }
            if(*s == '\r' || (*s == '\n' && gendoc_out > gendoc_buf && gendoc_out[-1] == '\n')) { s++; continue; }
            gendoc_text("%c", *s++); continue;
        }
        /* our <doc> tag, defining some variables. Skip that from output */
        if(!memcmp(s, "<doc>", 5)) {
            s += 5;
            for(e = s; *e && memcmp(e, "</doc>", 6); e++)
                if(*e == '\n') gendoc_l++;
            gendoc_doc(s, e);
            s = e + 6;
        } else
        /* hello page markers */
        if(!memcmp(s, "<hello>", 7)) { s += 7; gendoc_h = gendoc_H = 1; gendoc_last = gendoc_first = -2; } else
        if(!memcmp(s, "</hello>", 8)) { s += 8; gendoc_h = 0; } else
        /* parse headings and collect Table of Contents info */
        if(s[0] == '<' && s[1] == 'h' && s[2] >= '1' && s[2] <= '6') {
            s++;
            for(e = s; *e && (e[0] != '<' || e[1] != '/' || e[2] != 'h'); e++)
                if(*e == '\n') gendoc_l++;
            gendoc_heading(s, e);
            s = e + 5;
        } else
        /* Table of Contents caption, just adds to the toc, but does not generate output */
        if(!memcmp(s, "<cap>", 5)) {
            s += 5;
            for(e = s; *e && memcmp(e, "</cap>", 6); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_toc = (char**)myrealloc(gendoc_toc, (gendoc_numtoc + 1) * 4 * sizeof(char*));
            gendoc_toc[gendoc_numtoc * 4 + 0] = NULL;
            gendoc_toc[gendoc_numtoc * 4 + 1] = NULL;
            gendoc_toc[gendoc_numtoc * 4 + 2] = strdupl(s);
            gendoc_toc[gendoc_numtoc * 4 + 3] = NULL;
            gendoc_numtoc++;
            s = e + 6;
        } else
        /* styling text */
        if(!memcmp(s, "<b>", 3)) { s += 3; gendoc_text("<b>"); gendoc_chk_o(T_b); } else
        if(!memcmp(s, "</b>", 4)) { s += 4; gendoc_text("</b>"); gendoc_chk_c(T_b); } else
        if(!memcmp(s, "<i>", 3)) { s += 3; gendoc_text("<i>"); gendoc_chk_o(T_i); } else
        if(!memcmp(s, "</i>", 4)) { s += 4; gendoc_text("</i>"); gendoc_chk_c(T_i); } else
        if(!memcmp(s, "<u>", 3)) { s += 3; gendoc_text("<u>"); gendoc_chk_o(T_u); } else
        if(!memcmp(s, "</u>", 4)) { s += 4; gendoc_text("</u>"); gendoc_chk_c(T_u); } else
        if(!memcmp(s, "<s>", 3)) { s += 3; gendoc_text("<s>"); gendoc_chk_o(T_s); } else
        if(!memcmp(s, "</s>", 4)) { s += 4; gendoc_text("</s>"); gendoc_chk_c(T_s); } else
        if(!memcmp(s, "<sup>", 5)) { s += 5; gendoc_text("<sup>"); gendoc_chk_o(T_sup); } else
        if(!memcmp(s, "</sup>", 6)) { s += 6; gendoc_text("</sup>"); gendoc_chk_c(T_sup); } else
        if(!memcmp(s, "<sub>", 5)) { s += 5; gendoc_text("<sub>"); gendoc_chk_o(T_sub); } else
        if(!memcmp(s, "</sub>", 6)) { s += 6; gendoc_text("</sub>"); gendoc_chk_c(T_sub); } else
        if(!memcmp(s, "<quote>", 7)) { s += 7; gendoc_text("<blockquote class=\"pre\"><span></span>"); gendoc_chk_o(T_quote); } else
        if(!memcmp(s, "</quote>", 8)) { s += 8; gendoc_text("</blockquote>"); gendoc_chk_c(T_quote); } else
        /* structuring text */
        if(!memcmp(s, "<p>", 3)) { s += 3; gendoc_text("<p>"); gendoc_chk_o(T_p); } else
        if(!memcmp(s, "</p>", 4)) { s += 4; gendoc_text("</p>"); gendoc_chk_c(T_p); } else
        /* line breaks */
        if(!memcmp(s, "<br>", 4)) { s += 4; gendoc_text("<br>"); } else
        if(!memcmp(s, "<hr>", 4)) { s += 4; gendoc_text("<hr>"); } else
        /* lists */
        if(!memcmp(s, "<ol>", 4)) { s += 4; gendoc_text("<ol>"); gendoc_chk_o(T_ol); } else
        if(!memcmp(s, "</ol>", 5)) { s += 5; gendoc_text("</ol>"); gendoc_chk_c(T_ol); } else
        if(!memcmp(s, "<ul>", 4)) { s += 4; gendoc_text("<ul>"); gendoc_chk_o(T_ul); } else
        if(!memcmp(s, "</ul>", 5)) { s += 5; gendoc_text("</ul>"); gendoc_chk_c(T_ul); } else
        if(!memcmp(s, "<li>", 4)) { s += 4; gendoc_text("<li>"); gendoc_chk_o(T_li); } else
        if(!memcmp(s, "</li>", 5)) { s += 5; gendoc_text("</li>"); gendoc_chk_c(T_li); } else
        /* data fields */
        if(!memcmp(s, "<dl>", 4)) { s += 4; gendoc_text("<dl>"); gendoc_chk_o(T_dl); } else
        if(!memcmp(s, "</dl>", 5)) { s += 5; gendoc_text("</dl>"); gendoc_chk_c(T_dl); } else
        if(!memcmp(s, "<dt>", 4)) { s += 4; gendoc_text("<dt>"); gendoc_chk_o(T_dt); } else
        if(!memcmp(s, "</dt>", 5)) { s += 5; gendoc_text("</dt>"); gendoc_chk_c(T_dt); } else
        if(!memcmp(s, "<dd>", 4)) { s += 4; gendoc_text("<dd>"); gendoc_chk_o(T_dd); } else
        if(!memcmp(s, "</dd>", 5)) { s += 5; gendoc_text("</dd>"); gendoc_chk_c(T_dd); } else
        /* grid, invisible table */
        if(!memcmp(s, "<grid>", 6)) { s += 6; gendoc_text("<table class=\"grid\">"); gendoc_chk_o(T_grid); } else
        if(!memcmp(s, "</grid>", 7)) { s += 7; gendoc_text("</table>"); gendoc_chk_c(T_grid); } else
        if(!memcmp(s, "<gr>", 4)) { s += 4; gendoc_text("<tr>"); gendoc_chk_o(T_gr); } else
        if(!memcmp(s, "</gr>", 5)) { s += 5; gendoc_text("</tr>"); gendoc_chk_c(T_gr); } else
        if(!memcmp(s, "<gd>", 4)) { s += 4; gendoc_text("<td>"); gendoc_chk_o(T_gd); } else
        if(!memcmp(s, "<gD>", 4)) { s += 4; gendoc_text("<td class=\"wide\">"); gendoc_chk_o(T_gd); } else
        if(!memcmp(s, "</gd>", 5)) { s += 5; gendoc_text("</td>"); gendoc_chk_c(T_gd); } else
        /* table, needs a surrounding div for scrollbars */
        if(!memcmp(s, "<table>", 7)) { s += 7; gendoc_text("<div class=\"table\"><table>"); gendoc_chk_o(T_table); } else
        if(!memcmp(s, "</table>", 8)) { s += 8; gendoc_text("</table></div>"); gendoc_chk_c(T_table); } else
        if(!memcmp(s, "<tr>", 4)) { s += 4; gendoc_text("<tr>"); gendoc_chk_o(T_tr); } else
        if(!memcmp(s, "</tr>", 5)) { s += 5; gendoc_text("</tr>"); gendoc_chk_c(T_tr); } else
        /* table cell header */
        if(!memcmp(s, "<th>", 4)) { s += 4; gendoc_text("<th>"); gendoc_chk_o(T_th); } else
        if(!memcmp(s, "<tH>", 4)) { s += 4; gendoc_text("<th class=\"wide\">"); gendoc_chk_o(T_th); } else
        if(!memcmp(s, "</th>", 5)) { s += 5; gendoc_text("</th>"); gendoc_chk_c(T_th); } else
        /* table cell data */
        if(!memcmp(s, "<td>", 4)) { s += 4; gendoc_text("<td>"); gendoc_chk_o(T_td); } else
        if(!memcmp(s, "<tD>", 4)) { s += 4; gendoc_text("<td class=\"wide\">"); gendoc_chk_o(T_td); } else
        if(!memcmp(s, "</td>", 5)) { s += 5; gendoc_text("</td>"); gendoc_chk_c(T_td); } else
        /* table cell aligned right (number) */
        if(!memcmp(s, "<tn>", 4)) { s += 4; gendoc_text("<td class=\"right\">"); gendoc_chk_o(T_td); } else
        if(!memcmp(s, "<tN>", 4)) { s += 4; gendoc_text("<td class=\"right wide\">"); gendoc_chk_o(T_td); } else
        if(!memcmp(s, "</tn>", 5)) { s += 5; gendoc_text("</td>"); gendoc_chk_c(T_td); } else
        /* internal link */
        if(!memcmp(s, "<a>", 3)) {
            s += 3;
            for(e = s; *e && memcmp(e, "</a>", 4); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_internal_link(s);
            s = e + 4;
        } else
        /* external link, open in a new tab */
        if(!memcmp(s, "<a ", 3)) {
            gendoc_chk_o(T_a);
            s += 3; while(*s == ' ') s++;
            for(e = s; *e && *e != '>'; e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            /* if url starts with '#', then insert it as an internal link, but without checks */
            if(s[0] == '#')
                gendoc_text("<a href=\"%s\" onclick=\"c('%s')\">", s, s + 1);
            else
                gendoc_text("<a href=\"%s\" target=\"new\">", s);
            s = e + 1;
        } else
        if(!memcmp(s, "</a>", 4)) { s += 4; gendoc_text("</a>"); gendoc_chk_c(T_a); } else
        /* add teletype tags to output without parsing */
        if(!memcmp(s, "<tt>", 4)) {
            s += 4;
            for(e = s; *e && memcmp(e, "</tt>", 5); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            if(e > s) {
                s = htmlspecialchars(s);
                if(s) { gendoc_text("<samp>%s</samp>", s); free(s); }
            }
            s = e + 5;
        } else
        /* pre is pretty much the same, but adds an optional div around for scrollbars */
        if(!memcmp(s, "<pre>", 5)) {
            s += 5;
            for(e = s; *e && memcmp(e, "</pre>", 6); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_text("<div class=\"pre\"><pre>");
            if(e > s) {
                s = preformat(s);
                if(s) { gendoc_text("%s", s); free(s); }
            }
            gendoc_text("</pre></div>");
            s = e + 6;
        } else
        /* code is similar to pre, but needs line numbers and a syntax highlighter */
        if(!memcmp(s, "<code", 5)) {
            s += 5;
            if(*s == ' ') {
                s++; d = s;
                while(*s && *s != '>') s++;
                if(*s == '>') *s++ = 0;
            } else {
                d = NULL; s++;
            }
            for(e = s; *e && memcmp(e, "</code>", 7); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_source_code(s, d);
            s = e + 7;
        } else
        /* user interface elements */
        if(!memcmp(s, "<ui", 3) && s[3] >= '1' && s[3] <= '6' && s[4] == '>') {
            gendoc_text("<span class=\"ui%c\">", s[3]); s += 5;
            gendoc_chk_o(T_ui);
        } else
        if(!memcmp(s, "</ui", 4) && s[4] >= '1' && s[4] <= '6' && s[5] == '>') {
            gendoc_text("</span>"); s += 6;
            gendoc_chk_c(T_ui);
        } else
        /* keyboard keys */
        if(!memcmp(s, "<kbd>", 5)) {
            s += 5;
            for(e = s; *e && memcmp(e, "</kbd>", 6); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            if(e > s) {
                s = htmlspecialchars(s);
                if(s) { gendoc_text("<kbd>%s</kbd>", s); free(s); }
            }
            s = e + 6;
        } else
        /* mouse button left, right and wheel "icons" */
        if(!memcmp(s, "<mbl>", 5)) { s += 5; gendoc_text("<span class=\"mouseleft\"></span>"); } else
        if(!memcmp(s, "<mbr>", 5)) { s += 5; gendoc_text("<span class=\"mouseright\"></span>"); } else
        if(!memcmp(s, "<mbw>", 5)) { s += 5; gendoc_text("<span class=\"mousewheel\"></span>"); } else
        /* parse our image tags (<imgt>, <imgl>, <imgr>, <imgc>, <imgw>), but not the original HTML <img> tags */
        if(!memcmp(s, "<img", 4) && s[4] != ' ' && s[4] != '/' && s[4] != '>') {
            d = s + 4; s += 6;
            for(e = s; *e && *e != '>'; e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_image(*d, s);
            s = e + 1;
        } else
        /* image caption */
        if(!memcmp(s, "<fig>", 5)) {
            s += 5;
            for(e = s; *e && memcmp(e, "</fig>", 6); e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            if(e > s)
                gendoc_text("<span class=\"fig\">%s</span>", s);
            s = e + 6;
        } else
        /* alert boxes */
        if(!memcmp(s, "<info>", 6) || !memcmp(s, "<hint>", 6) || !memcmp(s, "<note>", 6) || !memcmp(s, "<also>", 6) ||
          !memcmp(s, "<todo>", 6) || !memcmp(s, "<warn>", 6)) {
            gendoc_text("<div class=\"%s\"><p><span>%s</span></p><p>",
              s[1] == 'h' ? "hint" : (s[1] == 't' || s[1] == 'w' ? "warn" : "info"),
              gendoc_lang[s[1] == 'i' ? 8 : (s[1] == 'h' ? 9 : (s[1] == 'n' ? 10 : (s[1] == 'a' ? 11 : (s[1] == 't' ? 12 : 13))))]);
            s += 6;
            gendoc_chk_o(T_alert);
        } else
        if(!memcmp(s, "</info>", 7) || !memcmp(s, "</hint>", 7) || !memcmp(s, "</note>", 7) || !memcmp(s, "</also>", 7) ||
          !memcmp(s, "</todo>", 7) || !memcmp(s, "</warn>", 7)) {
            gendoc_text("</p></div>");
            s += 7;
            gendoc_chk_c(T_alert);
        } else
        /* include another input file */
        if(!memcmp(s, "<include ", 9)) {
            s += 9;
            for(e = s; *e && *e != '>'; e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_include(s);
            s = e + 1;
        } else
        /* generate api documentation */
        if(!memcmp(s, "<api ", 5)) {
            s += 5;
            for(d = s; *s && *s != ' ' && *s != '>'; s++)
                if(*s == '\n') gendoc_l++;
            if(*s == '>') d = NULL; else *s++ = 0;
            for(e = s; *e && *e != '>'; e++)
                if(*e == '\n') gendoc_l++;
            *e = 0;
            gendoc_api(d, s);
            s = e + 1;
        } else {
            /* copy all the other tags to the output untouched */
            e = s;
            for(s++; *s && *s != '<' && s[-1] != '>'; s++)
                if(*s == '\n') gendoc_l++;
            tmp[0] = *s; *s = 0;
            gendoc_text("%s", e);
            for(d = e; d < s; d++) if(*d == '\n') { *d = 0; break; }
            fprintf(stderr, "gendoc warning: %s:%u: not gendoc compatible tag '%s'\n", gendoc_path, gendoc_l, e);
            *s = tmp[0];
        }
    }
}

/**
 * Include another source document.
 * @param string file name
 */
void gendoc_include(char *fn)
{
    char p[PATH_MAX], *s, *buf;
    unsigned int size;
    int n, l;

    strcpy(p, gendoc_path);
    n = gendoc_fn;
    l = gendoc_l;
    strcpy(gendoc_path + gendoc_fn, fn);
    s = strrchr(gendoc_path, '/');
    if(!s) s = gendoc_path; else s++;
    gendoc_fn = s - gendoc_path;
    buf = file_get_contents(gendoc_path, &size);
    s = strrchr(fn, '.');
    if(buf && s && !strcmp(s, ".md")) {
        for(s = buf; s < buf + size && *s; s++)
            if(*s == '\r')
                memcpy(s, s + 1, --size - (intptr_t)(s - buf));
        md_parse(buf, buf + size, 1);
        free(buf); buf = md_buf;
        md_buf = md_out = NULL; md_buflen = 0;
    }
    if(buf) {
        gendoc_l = 1;
        gendoc_parse(buf);
        free(buf);
    } else
        fprintf(stderr, "gendoc error: %s:0 unable to read\n", gendoc_path);
    gendoc_l = l;
    gendoc_fn = n;
    strcpy(gendoc_path, p);
}

/**
 * Output documentation.
 * @param stream to write to
 */
void gendoc_output(FILE *f)
{
    char *theme, title[256], *titimg = NULL, mime[8] = { 0 }, *s;
    unsigned char *buf = NULL;
    int i, w, h, H;

    (void)f;
    if(!gendoc_toc || gendoc_numtoc < 1) {
        fprintf(stderr, "gendoc error: no table of contents detected\n");
        gendoc_err++;
    }
    if(gendoc_fwd) {
        for(i = 0; i < gendoc_numfwd; i++) {
            fprintf(stderr, "gendoc error: %s\n", &gendoc_fwd[i][strlen(gendoc_fwd[i]) + 1]);
            gendoc_err++;
            free(gendoc_fwd[i]);
        }
        free(gendoc_fwd);
    }
    if(gendoc_buf) {
        if(gendoc_out)
            while(gendoc_out > gendoc_buf && (gendoc_out[-1] == ' ' || gendoc_out[-1] == '\r' || gendoc_out[-1] == '\n'))
                gendoc_out--;
        gendoc_prev_link();
        gendoc_text("</div>");
    }
    /* load user specified theme */
    theme = "hr,table,th,td{border-color:#e1e4e5;}\n"
        "th{background:#d6d6d6;}\n"
        "tr:nth-child(odd){background:#f3f6f6;}\n"
        "a{text-decoration:none;color:#2980B9;}\n"
        ".content{background:#fcfcfc;color:#404040;font-family:Lato,Helvetica,Neue,Arial,Deja Vu,sans-serif;}\n"
        ".title,.home,h1>a,h2>a,h3>a,h4>a,h5>a,h6>a{background:#2980B9;color:#fcfcfc;}\n"
        ".version{color:rgba(255,255,255,0.3);}\n"
        ".search{border:1px solid #2472a4;background:#fcfcfc;}\n"
        ".nav{background:#343131;color:#d9d9d9;}\n"
        ".nav p{color:#55a5d9;}\n"
        ".nav label:hover,.nav a:hover{background:#4e4a4a;}\n"
        ".nav .current{background:#fcfcfc;color:#404040;}\n"
        ".nav li>ul>li{background:#e3e3e3;}\n"
        ".nav li>ul>li>a{color:#404040;}\n"
        ".nav li>ul>li>a:hover{background:#d6d6d6;}\n"
        ".pre {border:1px solid #e1e4e5;background:#f8f8f8;}\n"
        ".info{background:#e7f2fa;}\n"
        ".info>p:first-child{background:#6ab0de;color:#fff;}\n"
        ".hint{background:#dbfaf4;}\n"
        ".hint>p:first-child{background:#1abc9c;color:#fff;}\n"
        ".warn{background:#ffedcc;}\n"
        ".warn>p:first-child{background:#f0b37e;color:#fff;}\n"
        ".btn{background:#f3f6f6;}\n"
        ".btn:hover{background:#e5ebeb;}\n"
        ".hl_h{background-color:#ccffcc;}\n"
        ".hl_c{color:#808080;font-style:italic;}\n"
        ".hl_p{color:#1f7199;}\n"
        ".hl_o{color:#404040;}\n"
        ".hl_n{color:#0164eb;}\n"
        ".hl_s{color:#986801;}\n"
        ".hl_t{color:#60A050;}\n"
        ".hl_k{color:#a626a4;}\n"
        ".hl_f{color:#2a9292;}\n"
        ".hl_v{color:#e95649;}\n";
    if(gendoc_theme[0]) {
        theme = file_get_contents(gendoc_theme, NULL);
        if(!theme) { fprintf(stderr, "gendoc error: %s:0: unable to read theme css\n", gendoc_theme); gendoc_err++; }
    }
    /* load title image */
    if(gendoc_lang[1][0]) strcpy(title, gendoc_lang[1]);
    if(gendoc_lang[1][0] && gendoc_lang[2][0]) { strcat(title, " "); strcat(title, gendoc_lang[2]); } else
    if(gendoc_lang[2][0]) strcpy(title, gendoc_lang[2]); else strcpy(title, "No name");
    if(gendoc_titleimg[0]) {
        buf = detect_image(gendoc_titleimg, mime, &w, &h, &i);
        if(buf) { titimg = base64_encode(buf, i); free(buf); }
    }
    /*** output html ***/
    fprintf(f, "<!DOCTYPE html>\n<html lang=\"%s\">\n<head>\n  <meta charset=\"utf-8\">\n"
    "  <meta name=\"generator\" content=\"gendoc " GENDOC_VERSION ": https://gitlab.com/bztsrc/gendoc\">\n"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "  <title>%s</title>\n", gendoc_lang[0], title);
    /* embedded stylesheet licensed under CC-BY */
    fprintf(f, "  <style rel=\"logic\">*{box-sizing:border-box;font-family:inherit;}"
    "body {background:rgba(0,0,0,0.05);font-weight:400;font-size:16px;}"
    "hr{display:block;height:1px;border:0;border-top:1px solid #e1e4e5;margin:26px 0;padding:0;border-top:1px solid;}"
    "br:after,br:before{display:table;content:\"\"}br{clear:both;}"
    "h1,h2,h3,h4,h5,h6{clear:both;margin:0px 0px 20px 0px;padding-top:4px;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale;}"
    "p{margin:0 0 24px}a{cursor:pointer;}"
    "h1{font-size:175%%}h2{font-size:150%%}h3{font-size:125%%}h4{font-size:115%%}h5{font-size:110%%}h6{font-size:100%%}"
    "pre,samp,code,var,kbd{font-family:Monaco,Consolas,Liberation Mono,Courier,monospace;font-variant-ligatures:none;}"
    "pre,code{display:block;overflow:auto;white-space:pre;font-size:14px;line-height:16px!important;}pre{padding:12px;margin:0px;}"
    "code{padding:0 0 12px 0;margin:12px 12px 0px 2px;background:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAAgCAYAAADT5RIaAAAAFklEQVQI12NgYGDgZWJgYGCgDkFtAAAWnAAsyj4TxgAAAABJRU5ErkJggg==) 0 0 repeat;}"
    ".lineno{display:block;padding:0px 4px 0px 4px;margin:12px 0px 0px 0px;opacity:.4;text-align:right;float:left;white-space:pre;font-size:12px;line-height:16px!important;}"
    "pre .hl_b,samp .hl_b,code .hl_b{display:block;}"
    "blockquote{margin:0px;padding:12px;}blockquote>span:first-child::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAgCAYAAABU1PscAAABRElEQVRYw+3WTytEURjH8c/4l1JTpKxsyEbYWFkpG+UVKOWFeAts7eytbJSysLLVxIKNFAslwhSlUQabUdM0xm0e967Ot+7inPN8z+13z73nXBKJRCIRoJSxbggrmMMInlHBIWoF+KEAQ9jAaJuxe2zhJUe/Iz0ZahZ/uTmMYT1nPxxg/I/xWQzn6IcD1IIha//wkEIBKhlq+nP0wwHOsYuvDjWvOfod6c1Yd9O4ZjDQZvwAbzn6oRVofpKbqLf0P+GxAD8cAO7w0NJ3WqAfDlBCuan9gaMC/XCA+cbJ+sMRqgX6oQCTWGtqX2O/QL8tfRlqyljGUlPgW2y3+SDz8Lv6mRvEQmPbm25ZqQvs/LHtRf3wCkxgtaWvij2cZJg36ocD/Pw91nGJY5zhM+O8UT/8Ck01TswrvHcxb9RPJBKJRDF8AyNbWk4WFTIzAAAAAElFTkSuQmCC);float:left;vertical-align:top;}"
    ".ui1,.ui2,.ui3,.ui4,.ui5,.ui6{display:inline-block;height:24px!important;line-height:24px!important;padding:0px 4px;margin:-2px 0px -2px;}"
    "kbd{display:inline-block;font-weight:700;border:1px solid #888;height:24px!important;padding:0px 4px;margin:-2px 0px -2px;border-radius:4px;background-image:linear-gradient(#ddd 0%%,#eee 10%%,#bbb 10%%,#ccc 30%%,#fff 85%%,#eee 85%%,#888 100%%);}"
    ".mouseleft,.mouseright,.mousewheel{display:inline-block;min-width:16px;height:24px!important;padding:0px;margin:-2px 0px 0px 0px;vertical-align:middle;}"
    ".mouseleft::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACoqKj9/f2zs7O1tbWRkZGlpaWsrKybm5u2traNjY2Wlpanp6empqaYmJiPj4+3t7f5+fmjo6P19fXu7u6ZmZnx8fHi4uLm5ube3t7q6uqwsLC0tLS7u7u4uLi/v7/W1tbDw8PLy8vPz8/T09Pa2trHx8eIiIhERkShhqFGAAAAAXRSTlMAQObYZgAAAI5JREFUGNNV0EcCwjAMRFEB6R2DKSGQUBLp/idEjsG23m7+cgAMIsJWwR8Z235pumDTnkUbv+lg7CIfTqvSbTquxsGFfjWXLlwsdOFs+XC1EHAWOEwCh4/A4S1weAkcFoHDU0CI8zGQx1Cpe0BVAO0j0PIfiR4cnZjLdHP7abQ9VRVZnaZ1VvjfO42o7edfH3EoHZS6XE4AAAAASUVORK5CYII=);}"
    ".mouseright::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACoqKj9/f2zs7O1tbWRkZGlpaWsrKybm5u2traNjY2Wlpanp6empqaYmJiPj4+3t7f5+fmjo6P19fXu7u6ZmZnx8fHi4uLm5ube3t7q6uqwsLC0tLS7u7u4uLi/v7/W1tbDw8PLy8vPz8/T09Pa2trHx8eIiIhERkShhqFGAAAAAXRSTlMAQObYZgAAAI9JREFUGNNV0NkWgjAMRdGozFOxWgdEcYLk/7/Q0EpL9tNd5/ECzLRCIoJF20zdlsinTbRn5Eu0O8zIl/Jk+dAPR4uWUo6d5QNenBDOTghXJ4RRQMCnwOErcPgIHN4Ch0ng8BIQ4nxYyWOo9H1FVwDqsaL4j8T0nknmy0xz+2uMO1UXWZ2mdVbo8LtBNK2dP/2+KB2shyfVAAAAAElFTkSuQmCC);}"
    ".mousewheel::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACzs7OoqKisrKyRkZGlpaX9/f22trabm5uNjY2mpqanp6ePj4+1tbWYmJi3t7eWlpajo6OZmZmwsLD5+fnu7u7x8fHi4uLW1tbm5ube3t7T09Pq6ur19fW4uLi7u7u0tLTa2trPz8+/v7/Dw8PLy8vHx8eIiIhERkS4354xAAAAAXRSTlMAQObYZgAAAKdJREFUGNNV0NkagiAUhdHjPAECzWWlWdL7P2Fno/nJumHzXx4iMMI5YeivVVOX592k2vkfy/1CxvjL6L6KJAd9ZF+GVxP144Eh4B170kPHEPAOmtwFEPxw5E6A4AeHKyD4wWEABD84nAHBDw43QPCDwyvA4RPgMAU4vAOO0mLcKFJqzHPDNETisSH4HpntVzbDyazaLZSdj2qqsk6Suqw2d7fO2fnmP7kAJW9a/HbiAAAAAElFTkSuQmCC);}"
    "footer{width:100%%;padding:0 3.236em;}footer p{opacity:0.6;}footer small{opacity:0.5;}footer a{text-decoration:none;color:inherit;}footer a:hover{text-decoration:underline;}"
    "dl{margin:0 0 24px 0;padding:0px;}dt{font-weight:700;margin-bottom:12px;}dd{margin:0 0 12px 24px;}"
    ".table table{margin:0px;border-collapse:collapse;border-spacing:0;empty-cells:show;border:1px solid;width:100%%;}"
    "th{font-weight:700;padding:8px 16px;overflow:visible;vertical-align:middle;white-space:nowrap;border:1px solid;}th.wide{width:100%%;}"
    "td{padding:8px 16px;overflow:visible;vertical-align:middle;font-size:90%%;border:1px solid;}td.right{text-align:right;}"
    "table.grid{margin:0px;padding:0px;border:none!important;background:none!important;border-spacing:0;border:0px!important;empty-cells:show;width:100%%;}"
    "table.grid tr, table.grid td{margin:0px;padding:0px;overflow:hidden;vertical-align:top;background:none!important;border:0px!important;font-size:90%%;}"
    "div.frame{position:absolute;width:100%%;min-height:100%%;margin:0px;padding:0px;max-width:1100px;top:0px;left:0px;}"
    "#_m{margin-left:300px;min-height:100%%;}"
    "div.title{display:block;width:300px;padding-top:.809em;padding-bottom:.809em;margin-bottom:.809em;text-align:center;font-weight:700;}"
    "div.title>a{padding:4px 6px;margin-bottom:.809em;font-size:150%%;}div.title>a:hover{background:transparent;}"
    "div.title>a>img{max-width:280px;border:0px;padding:0px;margin:0px;}"
    "div.title input{display:none;width:270px;border-radius:50px;padding:6px 12px;font-size:80%%;box-shadow:inset 0 1px 3px #ddd;transition:border .3s linear;}"
    "div.title input:required:invalid{background:#fcfcfc url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAASFBMVEUAAAC6urq7u7y7vLy8vL3Dw8TLzM67urq8vL29vr69vr/AwMK5ubm5ubm4uLi5t7fAwMC/vr/FxMXBwsPExcW+u7vDw8S4uLiryZHFAAAAF3RSTlMA9vGttk4lfbWnpJQG0MpzYEIglFRCGzMa+EsAAAB0SURBVBjTbY5bCsMwDAQl2U6cR5s0fcz9b1oLpWBC52dhENqVRsmG5SIn60wwryEm0LQkbUacAveh5XGDh4uKfsQZlOqpJAkS5gHPUyzgYdeLjB6/H7lvGbzlssP2WDoRGGzxrVRD82sHRukZ/xhv7tns/QXrhgcaFdOKBwAAAABJRU5ErkJggg==) no-repeat 10px 50%%;}"
    "div.title input:focus{background:#fcfcfc!important;}"
    "div.version{margin-top:.4045em;margin-bottom:.809em;font-size:90%%;}"
    "nav.side {display:block;position:fixed;top:0;bottom:0;left:0;width:300px;overflow-x:hidden;overflow-y:hidden;min-height:100%%;font-weight:400;z-index:999;}"
    "nav.mobile {display:none;font-weight:bold;padding:.4045em .809em;position:relative;line-height:50px;text-align:center;font-size:100%%;*zoom:1;}"
    "nav a{color:inherit;text-decoration:none;display:block;}"
    "nav.side>div{position:relative;overflow-x:hidden;overflow-y:scroll;width:320px;height:100%%;padding-bottom:64px;}"
    "div.nav p{height:32px;line-height:32px;padding:0 1.618em;margin:12px 0px 0px 0px;font-weight:700;text-transform:uppercase;font-size:85%%;white-space:nowrap;-webkit-font-smoothing:antialiased}"
    "div.nav li>.current,div.nav li>ul{display:none;}"
    "div.nav li>a,div.nav li>label{display:block;}"
    "div.nav a,div.nav ul>li>label,div.nav ul>li>.current{width:300px;line-height:18px;padding:0.4045em 1.618em;}"
    "div.nav a,div.nav ul>li>label{cursor:pointer;}"
    "div.nav .current{font-weight:700;border-top:1px solid;border-bottom:1px solid #c9c9c9;}"
    "div.nav ul>li>ul>li>a{border-right:solid 1px #c9c9c9;font-size:90%%;}"
    "div.nav ul>li>ul>li.h2>a{padding:0.4045em 2.427em;}"
    "div.nav ul>li>ul>li.h3>a{padding:.4045em 1.618em .4045em 4.045em;}"
    "div.nav ul>li>ul>li.h4>a{padding:.4045em 1.618em .4045em 5.663em;}"
    "div.nav ul>li>ul>li.h5>a{padding:.4045em 1.618em .4045em 7.281em;}"
    "div.nav ul>li>ul>li.h6>a{padding:.4045em 1.618em .4045em 8.899em;}"
    "div.nav ul,div.nav li,.breadcrumbs{margin:0px!important;padding:0px;list-style:none;}"
    "ul.breadcrumbs,.breadcrumbs li{display:inline-block;}"
    ".menu{display:inline-block;position:absolute;top:12px;right:20px;cursor:pointer;width:1.5em;height:1.5em;vertical-align:middle;padding:16px 24px 16px 24px;border:solid 1px rgba(255, 255, 255, 0.5);border-radius:5px;background:no-repeat center center url(\"data:image/svg+xml;charset=utf8,%%3Csvg viewBox='0 0 30 30' xmlns='http://www.w3.org/2000/svg'%%3E%%3Cpath stroke='rgba(255, 255, 255, 0.5)' stroke-width='2' stroke-linecap='round' stroke-miterlimit='10' d='M4 7h22M4 15h22M4 23h22'/%%3E%%3C/svg%%3E\");}"
    ".home{display:inline-block;max-width:16px;max-height:16px;line-height:16px;margin:0 5px 0 0;cursor:pointer;}"
    ".home::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAS1BMVEUAAAD8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/PyxWsjVAAAAGHRSTlMAx6oLLRnYiXFEvPbey518NQbsj11QKhQ+J0ktAAAAbElEQVQY053ISQ7DMAwEwRYpavHurPz/SwMoMpD46L4MpvBTB0zTP+R9z55Ebh0yQL5DaDA+0WVR3h3Gikp9iPKF9MIkQhSDwUlGHCLQxhLGJkpLZcNYS/tdyorP/DQ7HgBqKRUgHBDcw2X4AFsJCSXB/5UVAAAAAElFTkSuQmCC);}"
    "h1>a,h2>a,h3>a,h4>a,h5>a,h6>a{display:none;max-width:16px;max-height:24px;margin:-8px 0 0 5px;vertical-align:middle;}"
    "h1:hover>a,h2:hover>a,h3:hover>a,h4:hover>a,h5:hover>a,h6:hover>a{display:inline-block;text-decoration:none!important;}"
    "h1>a::before,h2>a::before,h3>a::before,h4>a::before,h5>a::before,h6>a::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAiBAMAAACkb0T0AAAAMFBMVEUAAAD8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/PzS+N+gAAAAD3RSTlMABab5cVY73o4k2cJTPb1Q83MyAAAAh0lEQVQY02P4DwWkM7YxsNiDGJ8EGBgmgxiKDDkCLAwgAQn7hQxARiGj8v8/IIYDi2a9GZChHiAnMLmBjeGTaIBUAMcFaYaNvA2sC1kD9RkSOCwZF7IW/WdgSP4+yYHlP5Bx/r+RgDiIIfW5kVEfyHBgPMYgBLLdkoEBKABkfHZgyIa7h04MAOVty/RC/PhoAAAAAElFTkSuQmCC);}"
    "h1>a:hover::after,h2>a:hover::after,h3>a:hover::after,h4>a:hover::after,h5>a:hover::after,h6>a:hover::after{"
    "content:\"%s\";display:block;padding:12px;position:absolute;margin:-8px 8px;font-weight:400;font-size:14px;background:rgba(0,0,0,.8);color:#fff;border-radius:4px;}"
    "input[type=radio]{display:none;}"
    "input[type=radio]:checked ~ ul{display:block;}"
    ".fig{margin-top:-12px;padding-bottom:12px;display:block;text-align:center;font-style:italic;}"
    "div.page{width:100%%;padding:1.618em 3.236em;margin:auto;line-height:24px;}"
    "div.page ol{margin:0 0 24px 12px;padding-left:0px;}div.page ul{margin:0 0 24px 24px;list-style:disc outside;padding-left:0px;}"
    "div.page ol{list-style-type:none;counter-reset:list;}div.page ol li:before{counter-increment:list;content:counters(list,\".\") \". \";}"
    "div.pre{overflow-x:auto;margin:1px 0px 24px;}div.table{overflow-x:auto;margin:0px 0px 24px;}"
    "div.info,div.hint,div.warn{padding:12px;line-height:24px;margin-bottom:24px;}"
    "div.info>p,div.hint>p,div.warn>p{margin:0px;}"
    "div.info>p:first-child,div.hint>p:first-child,div.warn>p:first-child{display:block;font-weight:700;padding:2px 8px 2px;margin:-12px -12px 8px -12px;vertical-align:middle;}"
    "div.info>p:first-child>span,div.hint>p:first-child>span,div.warn>p:first-child>span{display:block;max-height:20px;margin:0px;vertical-align:middle;}"
    "div.info>p:first-child>span::before,div.hint>p:first-child>span::before,div.warn>p:first-child>span::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAApUlEQVQ4y82TwQnDMAxFpVJyLPiSbbJETr13Fo+RZZxLRghkgnSEEni9ONS0sg1tDv3wwfjbX7Iki/wVAAd4IPBCiHuudrkHVvJYgb50eaOO7cMkpv0eeQGukYuRiUsNvBFpSvTJ0P2un0Sk+6Le3WEG58yBFrjt61r7Qqbij0gLIX3CaPgOqtqoaiMig6GPtTbOwCVyLraxMEj3yPIgHTLKv3ymJySzt16bW/sWAAAAAElFTkSuQmCC);}"
    "p>div:last-child,dd>*:last-child,td>*:last-child,li>ol,li>ul{margin-bottom:0px!important;}"
    "img{border:0px;}img.imgt{display:inline-block;max-height:22px!important;padding:0px;margin:-4px 0px 0px 0px;vertical-align:middle;}img.imgl{float:left;margin:0px 12px 12px 0px;}img.imgr{float:right;margin:0px 0px 12px 12px;}div.imgc{text-align:center;padding:0px;margin:0 0 12px 0;clear:both;}img.imgc{max-width:100%%;}img.imgw{width:100%%;margin-bottom:12px;clear:both;}"
    ".btn{border-radius:2px;line-height:normal;white-space:nowrap;color:inherit;text-align:center;cursor:pointer;font-size:100%%;padding:4px 12px 8px;border:1px solid rgba(0,0,0,.1);text-decoration:none;box-shadow:inset 0 1px 2px -1px hsla(0,0%%,100%%,.5),inset 0 -2px 0 0 rgba(0,0,0,.1);vertical-align:middle;*zoom:1;user-select:none;transition:all .1s linear}"
    ".prev{float:left;}.prev::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABIAAAASCAMAAABhEH5lAAAANlBMVEUAAABAQEBAQEBAQEBAQEBAQEBAQEBBQUFAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAWW5SEAAAAEnRSTlMA/fC9r2kXAjMN34F3ZlUu6B40Y5wGAAAAVElEQVQY08XPSw6AIAwEUIbS8lFE739Zq6luGtbM8iXTTMOCZGECiCX/MhIQTyCNz+SRrUc1MWKVvR5KYCN6pUFDRtqq4Sql9IYJ+aI/70f4qdOHblOhAuUcC5KnAAAAAElFTkSuQmCC);}"
    ".next{float:right;}.next::after{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABIAAAASBAMAAACk4JNkAAAAJFBMVEUAAABAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEC4lvDfAAAAC3RSTlMAx711ZjlFPh3zLASjkrYAAABDSURBVAjXY6AUsGhvcgAzOKR3797YAGIx7t5mvVsAxPLevZ159xYQS3v3zgLrTSDW7tTQBcy7ESyELEIHwhSEyQjbAAH1HsMY8tCHAAAAAElFTkSuQmCC);}"
    "@media screen and (max-width:991.98px){nav.mobile{display:block;}nav.side{display:none;}#menuchk:checked ~ nav.side{display:block;}#_m{margin-left:0px;}}",
    gendoc_lang[7]);
    for(i = 0; i < gendoc_numtoc; i++)
        if(gendoc_toc[i * 4 + 0] && gendoc_toc[i * 4 + 0][0] && (intptr_t)gendoc_toc[i * 4 + 1] == '1')
            fprintf(f, "#_%s:checked ~ nav div ul li[rel=%s]>.toc,", gendoc_toc[i * 4 + 0], gendoc_toc[i * 4 + 0]);
    fprintf(f, "div.page{display:none;}");
    for(i = 0; i < gendoc_numtoc; i++)
        if(gendoc_toc[i * 4 + 0] && gendoc_toc[i * 4 + 0][0] && (intptr_t)gendoc_toc[i * 4 + 1] == '1')
            fprintf(f, "#_%s:checked ~ nav div ul li[rel=%s]>ul,#_%s:checked ~ nav div ul li[rel=%s]>.current,#_%s:checked ~ div div[rel=%s],",
                gendoc_toc[i * 4 + 0], gendoc_toc[i * 4 + 0], gendoc_toc[i * 4 + 0],
                gendoc_toc[i * 4 + 0], gendoc_toc[i * 4 + 0], gendoc_toc[i * 4 + 0]);
    s = trimnl(theme);
    fprintf(f, "#_:checked ~ div div[rel=_]{display:block;}</style>\n  <style rel=\"theme\">%s</style>\n</head>\n<body>\n  <div class=\"frame content\">\n    ", s ? s : "");
    if(s) free(s);
    if(gendoc_theme[0] && theme) free(theme);
    if(gendoc_H) fprintf(f, "<input type=\"radio\" name=\"page\" id=\"_\" checked>");
    for(i = 0; i < gendoc_numtoc; i++)
        if(gendoc_toc[i * 4 + 0] && gendoc_toc[i * 4 + 0][0] && (intptr_t)gendoc_toc[i * 4 + 1] == '1') {
            fprintf(f, "<input type=\"radio\" name=\"page\" id=\"_%s\"%s>", gendoc_toc[i * 4 + 0], !gendoc_H ? " checked" : "");
            gendoc_H = 1;
        }
    if(gendoc_H) fprintf(f, "\n");
    fprintf(f, "    <input type=\"checkbox\" id=\"menuchk\" style=\"display:none;\"><nav class=\"side nav\"><div>\n      <div class=\"title\"><a href=\"%s\">", gendoc_lang[3]);
    if(titimg) {
        s = htmlspecialchars(gendoc_lang[1]);
        fprintf(f, "<img alt=\"%s\" ", s ? s : "");
        if(s) free(s);
        fprintf(f, " src=\"data:image/%s;base64,%s\">", mime, titimg);
        if(gendoc_lang[2][0]) fprintf(f, " %s", gendoc_lang[2]);
        free(titimg);
    } else
        fprintf(f, "%s", title);
    fprintf(f, "</a><div class=\"version\">%s</div>"
        "<input id=\"_q\" class=\"search\" type=\"text\" required=\"required\" onkeyup=\"s(this.value);\"></div>"
        "      <div id=\"_s\" class=\"nav\"></div>\n      <div id=\"_t\" class=\"nav\">\n", gendoc_lang[4]);
    H = 0;
    for(i = 0; i < gendoc_numtoc; i++)
        if(gendoc_toc[i * 4 + 2] && gendoc_toc[i * 4 + 2][0]) {
            if(!gendoc_toc[i * 4 + 1]) {
                if(H > 1) fprintf(f, "        </ul></li>\n");
                if(H) fprintf(f, "        </ul>\n");
                fprintf(f, "        <p>%s</p>\n", gendoc_toc[i * 4 + 2]);
                H = 0;
            } else {
                if((intptr_t)gendoc_toc[i * 4 + 1] == '1') {
                    if(!H) fprintf(f, "        <ul>\n");
                    else fprintf(f, "        </ul></li>\n");
                    fprintf(f, "        <li rel=\"%s\"><label class=\"toc\" for=\"_%s\">%s</label><div class=\"current\">%s</div><ul>\n",
                        gendoc_toc[i * 4 + 0] ? gendoc_toc[i * 4 + 0] : "",
                        gendoc_toc[i * 4 + 0] ? gendoc_toc[i * 4 + 0] : "",
                        gendoc_toc[i * 4 + 2], gendoc_toc[i * 4 + 2]);
                } else
                    fprintf(f, "          <li class=\"h%c\"><a href=\"#%s\" onclick=\"m()\">%s</a></li>\n",
                        (char)((intptr_t)gendoc_toc[i * 4 + 1]),
                        gendoc_toc[i * 4 + 0] ? gendoc_toc[i * 4 + 0] : "",
                        gendoc_toc[i * 4 + 2]);
                H = (intptr_t)gendoc_toc[i * 4 + 1] - '0';
            }
        }
    if(H > 1) fprintf(f, "        </ul></li>\n");
    if(H) fprintf(f, "        </ul>\n");
    fprintf(f, "      </div>\n"
        "    </div></nav>\n"
        "    <div id=\"_m\">\n"
        "      <nav class=\"mobile title\">%s<label for=\"menuchk\" class=\"menu\"></label></nav>\n", title);
    if(gendoc_buf) {
        for(s = gendoc_buf; s < gendoc_out && (*s == ' ' || *s == '\r' || *s == '\n'); s++);
        fwrite(s, 1, (intptr_t)gendoc_out - (intptr_t)s, f);
        free(gendoc_buf);
    }
    fprintf(f, "\n      <footer><hr><p>© Copyright %s<br><small>Generated by <a href=\"https://gitlab.com/bztsrc/gendoc\">gendoc</a> v" GENDOC_VERSION "</small></p></footer>\n"
        "    </div>\n"
        "  </div>\n", gendoc_lang[18]);
    /* embedded JavaScript, optional, minimal, vanilla (jQuery-less) code. Licensed under CC-BY */
    fprintf(f, "<script>"
        "function m(){document.getElementById(\"menuchk\").checked=false;}"
        /* onclick handler that changes the url *and* switches pages too */
        "function c(s){"
          "var r=document.getElementById(s);"
          "if(r!=undefined){"
            "if(r.tagName==\"INPUT\")r.checked=true;"
            "else document.getElementById(\"_\"+r.parentNode.getAttribute(\"rel\")).checked=true;"
          "}m();}");
        /* search function */
    fprintf(f, "function s(s){"
          "var r=document.getElementById(\"_s\"),p=document.getElementById(\"_m\").getElementsByClassName(\"page\"),n,i,j,a,b,c,d;"
          "if(s){"
            "s=s.toLowerCase();document.getElementById(\"_t\").style.display=\"none\";r.style.display=\"block\";"
            "while(r.firstChild)r.removeChild(r.firstChild);n=document.createElement(\"p\");n.appendChild(document.createTextNode(\"%s\"));r.appendChild(n);"
            "for(i=1;i<p.length;i++){"
              "a=p[i].getAttribute(\"rel\");b=\"\";c=p[i].childNodes;d=p[i].getElementsByTagName(\"H1\")[0].innerText;", gendoc_lang[5]);
    fprintf(f, "for(j=1;j<c.length && c[j].className!=\"btn prev\";j++){"
                "if(c[j].id!=undefined&&c[j].id!=\"\"){"
                  "a=c[j].id;d=c[j].innerText;"
                "}else if(a!=b&&c[j].innerText!=undefined&&c[j].innerText.toLowerCase().indexOf(s)!=-1){"
                  "b=a;n=document.createElement(\"a\");n.appendChild(document.createTextNode(d));n.setAttribute(\"href\",\"#\"+a);n.setAttribute(\"onclick\",\"c('\"+a+\"');\");r.appendChild(n);"
                "}}}"
          "}else{"
            "document.getElementById(\"_t\").style.display=\"block\";r.style.display=\"none\";}}");
    fprintf(f, "document.addEventListener(\"DOMContentLoaded\",function(e){var i,r,n;document.getElementById(\"_q\").style.display=\"inline-block\";"
        /* the query URL hack */
        "if(document.location.href.indexOf(\"?\")!=-1)document.location.href=document.location.href.replace(\"?\",\"#\");else{"
        /* replace labels with "a" tags that change the url too */
        "r=document.querySelectorAll(\"LABEL:not(.menu)\");"
        "while(r.length){"
        "l=r[0].getAttribute(\"for\").substr(1);");
    fprintf(f, "n=document.createElement(\"a\");n.appendChild(document.createTextNode(r[0].innerText));"
        "n.setAttribute(\"href\",\"#\"+l);n.setAttribute(\"onclick\",\"c('\"+(l!=\"\"?l:\"_\")+\"');\");"
        "if(r[0].getAttribute(\"class\")!=undefined)n.setAttribute(\"class\",r[0].getAttribute(\"class\"));"
        "if(r[0].getAttribute(\"title\")!=undefined&&l!=\"\")n.setAttribute(\"title\",r[0].getAttribute(\"title\"));"
        "if(r[0].getAttribute(\"accesskey\")!=undefined)n.setAttribute(\"accesskey\",r[0].getAttribute(\"accesskey\"));"
        "r[0].parentNode.replaceChild(n,r[0]);"
        "r=document.querySelectorAll(\"LABEL:not(.menu)\");"
        /* switch to a page which is detected from the url */
        "}try{c(document.location.href.split(\"#\")[1]);}catch(e){}}});"
        "</script>\n</body>\n</html>\n");
    if(gendoc_toc) {
        for(i = 0; i < gendoc_numtoc; i++) {
            if(gendoc_toc[i * 4 + 0]) free(gendoc_toc[i * 4 + 0]);
            if(gendoc_toc[i * 4 + 2]) free(gendoc_toc[i * 4 + 2]);
            if(gendoc_toc[i * 4 + 3]) free(gendoc_toc[i * 4 + 3]);
        }
        free(gendoc_toc);
    }
    if(gendoc_rules) {
        for(i = 0; i < gendoc_numrules; i++) {
            free(gendoc_rules[i * 9]);
            for(h = 1; h < 9; h++) {
                if(gendoc_rules[i * 9 + h]) {
                    for(w = 0; gendoc_rules[i * 9 + h][w]; w++)
                        free(gendoc_rules[i * 9 + h][w]);
                    free(gendoc_rules[i * 9 + h]);
                }
            }
        }
        free(gendoc_rules);
    }
}

/**
 * The main function
 */
int main(int argc, char **argv)
{
    FILE *f;
    DIR *dir;
    struct dirent *ent;
    char *fn, *buf, *s, *d, *e;
    int i, j, k;

    /*** collect info ***/
    if(argc < 3) {
        printf("./gendoc <output.html> <input file> [input file...]\n");
        return 0;
    }
    f = fopen(argv[1], "wb");
    if(!f) {
        fprintf(stderr, "gendoc error: %s:0 unable to write file\n", argv[1]);
        return 1;
    }
    /* load all highlight rules */
    strcpy(gendoc_path, argv[0]);
    fn = strrchr(gendoc_path, '/');
    if(!fn) { strcpy(gendoc_path, "./"); fn = gendoc_path + 2; } else fn++;
    strcpy(fn, "plugins");
    if(!(dir = opendir(gendoc_path))) {
        strcpy(gendoc_path, "/usr/share/gendoc/plugins");
        fn = gendoc_path + 18;
        dir = opendir(gendoc_path);
    }
    if(dir) {
        fn[7] = '/';
        while ((ent = readdir(dir)) != NULL) {
            i = strlen(ent->d_name); if(i < 9 || memcmp(ent->d_name, "hl_", 3) || strcmp(ent->d_name + i - 5, ".json")) continue;
            strcpy(fn + 8, ent->d_name);
            buf = file_get_contents(gendoc_path, NULL);
            if(buf) {
                gendoc_rules = (char***)myrealloc(gendoc_rules, (gendoc_numrules + 1) * 9 * sizeof(char**));
                memset(&gendoc_rules[gendoc_numrules * 9], 0, 9 * sizeof(char**));
                gendoc_rules[gendoc_numrules * 9] = (char**)myrealloc(NULL, i - 7);
                memcpy((char*)gendoc_rules[gendoc_numrules * 9], ent->d_name + 3, i - 8);
                for(s = buf, i = k = 0, j = 1; *s && j < 9; s++) {
                    if(s[0] == '/' && s[1] == '*') { s += 2; while(*s && s[-2] != '*' && s[-1] != '/') s++; }
                    if(*s == '[') k++; else if(*s == ']') { k--; if(k == 1) { i = 0; j++; } } else
                    if(*s == '\"') {
                        s++; for(e = s; *e && *e != '\"'; e++) if(*e == '\\') e++;
                        gendoc_rules[gendoc_numrules * 9 + j] = (char**)myrealloc(gendoc_rules[gendoc_numrules * 9 + j],
                            (i + 2) * sizeof(char*));
                        gendoc_rules[gendoc_numrules * 9 + j][i] = d = (char*)myrealloc(NULL, e - s + 1);
                        gendoc_rules[gendoc_numrules * 9 + j][i + 1] = NULL;
                        for(; s < e; s++)
                            if(s[0] != '\\' || s[1] != '\"') *d++ = *s;
                        i++;
                    }
                }
                free(buf);
                gendoc_numrules++;
            }
        }
        closedir(dir);
    }
    /* initialize variables */
    memset(gendoc_chk_l, 0, sizeof(gendoc_chk_l));
    memset(gendoc_titleimg, 0, sizeof(gendoc_titleimg));
    memset(gendoc_theme, 0, sizeof(gendoc_theme));
    memset(gendoc_lang, 0, sizeof(gendoc_lang));
    strcpy(gendoc_lang[0], "en");
    strcpy(gendoc_lang[3], "#");
    strcpy(gendoc_lang[4], "stable");
    strcpy(gendoc_lang[5], "Search Results");
    strcpy(gendoc_lang[6], "Home");
    strcpy(gendoc_lang[7], "Permalink to this headline");
    strcpy(gendoc_lang[8], "Important");
    strcpy(gendoc_lang[9], "Hint");
    strcpy(gendoc_lang[10], "Note");
    strcpy(gendoc_lang[11], "See Also");
    strcpy(gendoc_lang[12], "To Do");
    strcpy(gendoc_lang[13], "Warning");
    strcpy(gendoc_lang[14], "Arguments");
    strcpy(gendoc_lang[15], "Return Value");
    strcpy(gendoc_lang[16], "Previous");
    strcpy(gendoc_lang[17], "Next");
    strcpy(gendoc_lang[18], "unknown");
    /* iterate on input files */
    for(i = 2; i < argc; i++) {
        memset(gendoc_path, 0, sizeof(gendoc_path));
        gendoc_fn = 0;
        gendoc_include(argv[i]);
    }
    /* generate output */
    gendoc_output(f);
    fclose(f);
    if(gendoc_err)
        fprintf(stderr, "gendoc: there were errors during generation, your documentation is although saved, but probably is incomplete!\n");
    return 0;
}
