#!/usr/bin/php
<?php
/*
 * gendoc.php
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

setlocale(LC_ALL, 'en_US.UTF8');
mb_internal_encoding('utf-8');
if($_SERVER['argc'] < 3) die("./gendoc.php <output file> <input file> [input file] [input file...]\r\n");

class gendoc {
    public static $version = "1.0.0";

    /*** default translations and variables if not set with <doc> ***/
    public static $lang = [
        "lang" => "en",
        "titleimg" => "",
        "title" => "",
        "url" => "#",
        "version" => "stable",
        "theme" => "",
        "rslt" => "Search Results",
        "home" => "Home",
        "link" => "Permalink to this headline",
        "info" => "Important",
        "hint" => "Hint",
        "note" => "Note",
        "also" => "See Also",
        "todo" => "To Do",
        "warn" => "Warning",
        "args" => "Arguments",
        "rval" => "Return Value",
        "prev" => "Previous",
        "next" => "Next",
        "copy" => "unknown"
    ];

    public static $rules = [];      /* syntax highlight rules lang => [ comments, pseudo, op, num, str, types, keywords ] */
    public static $writer = null;   /* the output writer plugin */
    public static $fn = "";         /* file name, for error reporting */
    public static $l = 0;           /* line number, for error reporting */
    public static $out = "";        /* output string */
    public static $toc = [];        /* table of contents, each entry  id => [ level, name ] */
    public static $err = 0;         /* number of errors */
    public static $H = 0;           /* set if there was a hello tag */
    public static $h = 0;           /* set if we are in hello tag right now */
    public static $fwd = [];        /* forward references */
    public static $vld = [];        /* validations */
    private static $n = 0;          /* number of captions */
    private static $first = "";     /* for linking pages */
    private static $last = "";
    private static $prev = "";
    private static $lsec = "";

    /**
     * Convert a string into an URL and id attribute-safe string.
     * Sometimes it sucks that internet was designed by English only speakers. gendoc class only method.
     * @param string input
     * @return string the converted string
     */
    public static function safeid($str)
    {
        return preg_replace("/[\ ]+/", '_', trim(preg_replace("/[^a-z0-9]/", ' ', strtr(strtolower(html_entity_decode(trim($str))),
        ["\r" => "", "\"" => "", "\'" => "", "#" => "", "?" => "", "/" => "", "&" => "", ";" => "",
        "À"=>"a","à"=>"a","Á"=>"a","á"=>"a","Â"=>"a","â"=>"a","Ã"=>"a","ã"=>"a","Ä"=>"a","ä"=>"a","Å"=>"a","å"=>"a","Æ"=>"ae","æ"=>"ae",
        "Ç"=>"c","ç"=>"c","È"=>"e","è"=>"e","É"=>"e","é"=>"e","Ê"=>"e","ê"=>"e","Ë"=>"e","ë"=>"e","Ì"=>"i","ì"=>"i","Í"=>"i","í"=>"i",
        "Î"=>"i","î"=>"i","Ï"=>"i","ï"=>"i","Ð"=>"d","ð"=>"d","Ñ"=>"n","ñ"=>"n","Ò"=>"o","ò"=>"o","Ó"=>"o","ó"=>"o","Ô"=>"o","ô"=>"o",
        "Õ"=>"o","õ"=>"o","Ö"=>"o","ö"=>"o","Ø"=>"o","ø"=>"o","Ù"=>"u","ù"=>"u","Ú"=>"u","ú"=>"u","Û"=>"u","û"=>"u","Ü"=>"u","ü"=>"u",
        "Ý"=>"y","ý"=>"y","Þ"=>"p","þ"=>"p","Ā"=>"a","ā"=>"a","Ă"=>"a","ă"=>"a","Ą"=>"a","ą"=>"a","Ć"=>"c","ć"=>"c","Ĉ"=>"c","ĉ"=>"c",
        "Ċ"=>"c","ċ"=>"c","Č"=>"c","č"=>"c","Ď"=>"d","ď"=>"d","Đ"=>"d","đ"=>"d","Ē"=>"e","ē"=>"e","Ĕ"=>"e","ĕ"=>"e","Ė"=>"e","ė"=>"e",
        "Ę"=>"e","ę"=>"e","Ě"=>"e","ě"=>"e","Ĝ"=>"g","ĝ"=>"g","Ğ"=>"g","ğ"=>"g","Ġ"=>"g","ġ"=>"g","Ģ"=>"g","ģ"=>"g","Ĥ"=>"h","ĥ"=>"h",
        "Ħ"=>"h","ħ"=>"h","Ĩ"=>"i","ĩ"=>"i","Ī"=>"i","ī"=>"i","Ĭ"=>"i","ĭ"=>"i","Į"=>"i","į"=>"i","İ"=>"i","i"=>"i","Ĳ"=>"ij","ĳ"=>"ij",
        "Ĵ"=>"j","ĵ"=>"j","Ķ"=>"k","ķ"=>"k","Ĺ"=>"l","ĺ"=>"l","Ļ"=>"l","ļ"=>"l","Ľ"=>"l","ľ"=>"l","Ŀ"=>"l","ŀ"=>"l","Ł"=>"l","ł"=>"l",
        "Ń"=>"n","ń"=>"n","Ņ"=>"n","ņ"=>"n","Ň"=>"n","ň"=>"n","Ŋ"=>"n","ŋ"=>"n","Ō"=>"o","ō"=>"o","Ŏ"=>"o","ŏ"=>"o","Ő"=>"o","ő"=>"o",
        "Œ"=>"ce","œ"=>"ce","Ŕ"=>"r","ŕ"=>"r","Ŗ"=>"r","ŗ"=>"r","Ř"=>"r","ř"=>"r","Ś"=>"s","ś"=>"s","Ŝ"=>"s","ŝ"=>"s","Ş"=>"s","ş"=>"s",
        "Š"=>"s","š"=>"s","Ţ"=>"t","ţ"=>"t","Ť"=>"t","ť"=>"t","Ŧ"=>"t","ŧ"=>"t","Ũ"=>"u","ũ"=>"u","Ū"=>"u","ū"=>"u","Ŭ"=>"u","ŭ"=>"u",
        "Ů"=>"u","ů"=>"u","Ű"=>"u","ű"=>"u","Ų"=>"u","ų"=>"u","Ŵ"=>"w","ŵ"=>"w","Ŷ"=>"y","ŷ"=>"y","Ÿ"=>"y","ÿ"=>"y","Ź"=>"z","ź"=>"z",
        "Ż"=>"z","ż"=>"z","Ž"=>"z","ž"=>"z","Ɓ"=>"b","ɓ"=>"b","Ƃ"=>"b","ƃ"=>"b","Ƅ"=>"b","ƅ"=>"b","Ɔ"=>"c","ɔ"=>"c","Ƈ"=>"c","ƈ"=>"c",
        "Ɖ"=>"ɖ","ɖ"=>"d","Ɗ"=>"d","ɗ"=>"d","Ƌ"=>"d","ƌ"=>"d","Ǝ"=>"e","ǝ"=>"e","Ə"=>"e","ə"=>"e","Ɛ"=>"e","ɛ"=>"e","Ƒ"=>"f","ƒ"=>"f",
        "Ɠ"=>"g","ɠ"=>"g","Ɣ"=>"y","ɣ"=>"y","Ɩ"=>"l","ɩ"=>"l","Ɨ"=>"i","ɨ"=>"i","Ƙ"=>"k","ƙ"=>"k","Ɯ"=>"w","ɯ"=>"w","Ɲ"=>"n","ɲ"=>"n",
        "Ɵ"=>"o","ɵ"=>"o","Ơ"=>"o","ơ"=>"o","Ƣ"=>"oj","ƣ"=>"oj","Ƥ"=>"p","ƥ"=>"p","Ʀ"=>"r","ʀ"=>"r","Ƨ"=>"s","ƨ"=>"s","Ʃ"=>"s","ʃ"=>"s",
        "Ƭ"=>"t","ƭ"=>"t","Ʈ"=>"t","ʈ"=>"t","Ư"=>"u","ư"=>"u","Ʊ"=>"u","ʊ"=>"u","Ʋ"=>"u","ʋ"=>"u","Ƴ"=>"y","ƴ"=>"y","Ƶ"=>"z","ƶ"=>"z",
        "Ʒ"=>"z","ʒ"=>"z","Ƹ"=>"z","ƹ"=>"z","Ƽ"=>"z","ƽ"=>"z","Ǆ"=>"dz","ǆ"=>"dz","ǅ"=>"dz","ǆ"=>"dz","Ǉ"=>"lj","ǉ"=>"lj","ǈ"=>"lj",
        "ǉ"=>"lj","Ǌ"=>"nj","ǌ"=>"nj","ǋ"=>"nj","ǌ"=>"nj","Ǎ"=>"a","ǎ"=>"a","Ǐ"=>"i","ǐ"=>"i","Ǒ"=>"o","ǒ"=>"o","Ǔ"=>"u","ǔ"=>"u",
        "Ǖ"=>"u","ǖ"=>"u","Ǘ"=>"u","ǘ"=>"u","Ǚ"=>"u","ǚ"=>"u","Ǜ"=>"u","ǜ"=>"u","Ǟ"=>"a","ǟ"=>"a","Ǡ"=>"a","ǡ"=>"a","Ǣ"=>"ae","ǣ"=>"ae",
        "Ǥ"=>"g","ǥ"=>"g","Ǧ"=>"g","ǧ"=>"g","Ǩ"=>"k","ǩ"=>"k","Ǫ"=>"o","ǫ"=>"o","Ǭ"=>"o","ǭ"=>"o","Ǯ"=>"z","ǯ"=>"z","Ǳ"=>"dz","ǳ"=>"dz",
        "ǲ"=>"dz","ǳ"=>"dz","Ǵ"=>"g","ǵ"=>"g","Ƕ"=>"hj","ƕ"=>"hj","Ƿ"=>"p","ƿ"=>"p","Ǹ"=>"n","ǹ"=>"n","Ǻ"=>"a","ǻ"=>"a","Ǽ"=>"ae","ǽ"=>"ae",
        "Ǿ"=>"o","ǿ"=>"ǿ","Ȁ"=>"a","ȁ"=>"a","Ȃ"=>"a","ȃ"=>"a","Ȅ"=>"e","ȅ"=>"e","Ȇ"=>"e","ȇ"=>"e","Ȉ"=>"i","ȉ"=>"i","Ȋ"=>"i","ȋ"=>"i",
        "Ȍ"=>"o","ȍ"=>"o","Ȏ"=>"o","ȏ"=>"o","Ȑ"=>"r","ȑ"=>"r","Ȓ"=>"r","ȓ"=>"r","Ȕ"=>"u","ȕ"=>"u","Ȗ"=>"u","ȗ"=>"u","Ș"=>"s","ș"=>"s",
        "Ț"=>"t","ț"=>"t","Ȝ"=>"z","ȝ"=>"z","Ȟ"=>"h","ȟ"=>"h","Ƞ"=>"n","ƞ"=>"n","Ȣ"=>"o","ȣ"=>"o","Ȥ"=>"z","ȥ"=>"z","Ȧ"=>"a","ȧ"=>"a",
        "Ȩ"=>"e","ȩ"=>"e","Ȫ"=>"o","ȫ"=>"o","Ȭ"=>"o","ȭ"=>"o","Ȯ"=>"o","ȯ"=>"o","Ȱ"=>"o","ȱ"=>"o","Ȳ"=>"y","ȳ"=>"y","Ⱥ"=>"a","ⱥ"=>"a",
        "Ȼ"=>"c","ȼ"=>"c","Ƚ"=>"l","ƚ"=>"l","Ⱦ"=>"t","ⱦ"=>"t","Ƀ"=>"b","ƀ"=>"b","Ʉ"=>"u","ʉ"=>"u","Ɇ"=>"e","ɇ"=>"e","Ɉ"=>"j","ɉ"=>"j",
        "Ɋ"=>"q","ɋ"=>"q","Ɍ"=>"r","ɍ"=>"r","Ɏ"=>"y","ɏ"=>"y"]))));
    }

    /**
     * Report an error. gendoc class only method.
     * @param string error message
     */
    public static function report_error($msg)
    {
        echo("gendoc error: ".self::$fn.":".self::$l.": ".$msg."\r\n");
        self::$err++;
    }

    /**
     * Generate API documentation. gendoc class only method.
     * @param string language
     * @param string file name
     */
    public static function api($c, $fn)
    {
        if(!empty(self::$fn)) $fn = dirname(self::$fn)."/".$fn;
        $s = @file_get_contents($fn);
        if(empty($s)) { self::report_error("unable to read source '".$fn."'"); return; }
        $ext = "api_".$c;
        $plugin = dirname(__FILE__)."/plugins/".$ext.".php";
        $ext = "gendoc_".$ext;
        if(file_exists($plugin))
            require_once $plugin;
        else $ext="";
        if(!empty($ext) && function_exists($ext))
            $ext($s);
        else {
            /* in lack of a plugin, a simple API doc generator */
            if(preg_match_all("/\/\*\*[^\*](.*?)\*\/[\n](.*?)$/ims", $s, $M, PREG_SET_ORDER)) {
                foreach($M as $m) {
                    self::data_list_open();
                    self::data_topic_open();
                    self::source_code(trim($m[2]), $c);
                    self::data_topic_close();
                    self::data_description_open();
                    $t = 0;
                    foreach(explode("\n", $m[1]) as $dsc) {
                        $dsc = htmlspecialchars(trim(preg_replace("/^[\ \t]*\*[\ \t]*/", "", $dsc)));
                        if(!empty($dsc)) {
                            if(substr($dsc, 0, 7) == "@param ") {
                                if(!$t) {
                                    $t = 1;
                                    self::table_open();
                                    self::table_row_open();
                                    self::table_header_open();
                                    self::text(self::$lang["args"]);
                                    self::table_header_close();
                                    self::table_row_close();
                                }
                                self::table_row_open();
                                self::table_cell_open();
                                self::text(trim(substr($dsc, 7)));
                                self::table_cell_close();
                                self::table_row_close();
                            } else
                            if(substr($dsc, 0, 8) == "@return ") {
                                if(!$t) {
                                    $t = 1;
                                    self::table_open();
                                }
                                self::table_row_open();
                                self::table_header_open();
                                self::text(self::$lang["rval"]);
                                self::table_header_close();
                                self::table_row_close();
                                self::table_row_open();
                                self::table_cell_open();
                                self::text(trim(substr($dsc, 8)));
                                self::table_cell_close();
                                self::table_row_close();
                            } else
                            if(!$t)
                                self::text($dsc." ");
                        }
                    }
                    if($t)
                        self::table_close();
                    self::data_description_close();
                    self::data_list_close();
                    self::line_break();
                }
            }
        }
    }

    /**
     * Include another source document. gendoc class only method.
     * @param string file name
     */
    public static function include($fn) {
        if(!empty(self::$fn)) $fn = dirname(self::$fn)."/".$fn;
        $s = @file_get_contents($fn);
        if(empty($s)) { echo("gendoc error: ".$fn.":0: unable to read\r\n"); self::$err++; return; }
        $oldfn = self::$fn; $oldl = self::$l;
        self::$fn = $fn;
        self::$l = 1;
        $l = strrpos($fn, '.');
        if($l !== false) {
            $ext = "_".substr($fn, $l + 1);
            $plugin = dirname(__FILE__)."/plugins/fmt".$ext.".php";
            $ext = "gendoc".$ext;
            if(file_exists($plugin))
                require_once $plugin;
            else {
                if($ext != "gendoc_xml")
                    self::report_error("there is no ".$ext." plugin to read this format.");
                $ext="";
            }
        }
        if(!empty($ext) && class_exists($ext)) {
            if(method_exists($ext, "parse"))
                $s = $ext::parse($s);
            else
                self::report_error("the ".$ext." plugin does not support reading files.");
        }
        if(!empty($s)) self::parse($s);
        self::$fn = $oldfn; self::$l = $oldl;
    }

    /**
     * Parse <doc> tag. gendoc class only method.
     * @param string with <doc> sub-tags
     */
    public static function doc($str)
    {
        if(preg_match_all("/<([^\/][^>]+)>([^<]+)*/ms", $str, $m, PREG_SET_ORDER))
            foreach($m as $M) {
                self::$lang[$M[1]] =
                    $M[1] == "theme" && !empty($M[2]) ? dirname(self::$fn)."/".$M[2] : (
                    $M[1] == "titleimg" && !empty($M[2]) ?
                        dirname(self::$fn)."/".substr($M[2], 0, strpos($M[2], " ")).substr($M[2], strpos($M[2], " ")) : $M[2]);
            }
    }

    /**
     * Start <hello> tag. gendoc class only method.
     */
    public static function hello_open()
    {
        self::$h = self::$H = 1;
        self::$last = self::$first = "_";
    }

    /**
     * End <hello> tag. gendoc class only method.
     */
    public static function hello_close()
    {
        self::$h = 0;
    }

    /**
     * Add a caption to TOC. gendoc class only method.
     * @param string caption's name
     */
    public static function caption($name)
    {
        if(!empty(self::$vld)) {
            foreach(self::$vld as $k => $v)
                if($v != 0)
                    self::report_error("unclosed ".$k." in section ".(empty(self::$lsec) ? "unknown" : self::$lsec));
            self::$vld = [];
        }
        self::$lsec = $name;
        self::$toc["!".self::$n++] = [ "0", $name ];
    }

    /**
     * Add a heading. Has to be implemented by writer plugins.
     * @param string "1" to "6"
     * @param string heading user-readable name
     * @param string heading label id
     */
    public static function heading($level, $name, $id = "", $alias = "")
    {
        if(intval($level) < 1 || intval($level) > 6) {
            self::report_error("invalid heading level");
            return;
        }
        $level .= "";
        $name = trim($name);
        if(!empty(self::$vld)) {
            foreach(self::$vld as $k => $v)
                if($v != 0)
                    self::report_error("unclosed ".$k." in section ".(empty(self::$lsec) ? "unknown" : self::$lsec));
            self::$vld = [];
        }
        self::$lsec = $name;
        if(empty($name)) {
            self::report_error("empty heading name");
            return;
        }
        if(self::$h) {
            if(!empty(self::$writer)) self::$writer->heading($level, "", $name);
            else {
                if($level == "1")
                    self::$out .= "<div class=\"page\" rel=\"_\">";
                self::$out .= "\n<h".$level.">".$name."</h".$level.">";
            }
        } else {
            $id = self::safeid(!empty($id) ? $id : $name); $alias = self::safeid($alias);
            self::$out = trim(self::$out);
            if($level == "1" && !empty(self::$out)) {
                self::prev_link();
                self::next_link($id, $name);
                self::$prev = self::$last;
                self::$last = $id;
            }
            if(empty($id)) {
                self::report_error("no id for heading (".$name.")");
            } else
            if(!empty(self::$toc[$id])) {
                self::report_error("id for heading isn't unique (".$id.")");
                $id = "";
            } else
                self::$toc[$id] = [ $level, $name ];
            if(!empty(self::$fwd[self::safeid($name)])) {
                self::resolve_link(self::safeid($name), $id);
            }
            if(!empty(self::$writer)) self::$writer->heading($level, $id, $name);
            else {
                if($level == "1") {
                    if(empty(self::$first)) self::$first = $id;
                    self::$out .= "<div class=\"page\"".(!empty($id) ? " rel=\"".$id."\"" : "")."><div><ul class=\"breadcrumbs\"><li><label class=\"home\" for=\"_".
                        (self::$first=='_'?"":self::$first)."\" ".
                        "title=\"".htmlspecialchars(self::$lang["home"])."\"></label>&nbsp;»</li><li>&nbsp;".$name."</li></ul><hr></div>";
                }
                self::$out .= "\n".(!empty($alias) ? "<span id=\"".$alias."\"></span>" : "")."<h".$level.(!empty($id) ? " id=\"".$id."\"" : "").">";
                self::$out .= $name.(!empty($id) ? "<a href=\"#".$id."\"></a>" : "")."</h".$level.">";
            }
        }
    }

    /**
     * Start a paragraph. Has to be implemented by writer plugins.
     */
    public static function paragraph_open()
    {
        @self::$vld["p"]++;
        if(!empty(self::$writer)) self::$writer->paragraph_open();
        else self::$out .= "<p>";
    }

    /**
     * End a paragraph. Has to be implemented by writer plugins.
     */
    public static function paragraph_close()
    {
        if(empty(self::$vld["p"]))
            self::report_error("cannot close, paragraph is not open");
        else {
            if(!empty(self::$writer)) self::$writer->paragraph_close();
            else self::$out .= "</p>";
            self::$vld["p"]--;
        }
    }

    /**
     * Start bold text. Has to be implemented by writer plugins.
     */
    public static function bold_open()
    {
        if(!empty(self::$vld["b"]))
            self::report_error("bold is already open");
        else {
            if(!empty(self::$writer)) self::$writer->bold_open();
            else self::$out .= "<b>";
            self::$vld["b"] = 1;
        }
    }

    /**
     * Stop bold text. Has to be implemented by writer plugins.
     */
    public static function bold_close()
    {
        if(empty(self::$vld["b"]))
            self::report_error("cannot close, bold is not open");
        else {
            if(!empty(self::$writer)) self::$writer->bold_close();
            else self::$out .= "</b>";
            unset(self::$vld["b"]);
        }
    }

    /**
     * Start italic text. Has to be implemented by writer plugins.
     */
    public static function italic_open()
    {
        if(!empty(self::$vld["i"]))
            self::report_error("italic is already open");
        else {
            if(!empty(self::$writer)) self::$writer->italic_open();
            else self::$out .= "<i>";
            self::$vld["i"] = 1;
        }
    }

    /**
     * Stop italic text. Has to be implemented by writer plugins.
     */
    public static function italic_close()
    {
        if(empty(self::$vld["i"]))
            self::report_error("cannot close, italic is not open");
        else {
            if(!empty(self::$writer)) self::$writer->italic_close();
            else self::$out .= "</i>";
            unset(self::$vld["i"]);
        }
    }

    /**
     * Start underlined text. Has to be implemented by writer plugins.
     */
    public static function underline_open()
    {
        if(!empty(self::$vld["u"]))
            self::report_error("underline is already open");
        else {
            if(!empty(self::$writer)) self::$writer->underline_open();
            else self::$out .= "<u>";
            self::$vld["u"] = 1;
        }
    }

    /**
     * Stop underlined text. Has to be implemented by writer plugins.
     */
    public static function underline_close()
    {
        if(empty(self::$vld["u"]))
            self::report_error("cannot close, underline is not open");
        else {
            if(!empty(self::$writer)) self::$writer->underline_close();
            else self::$out .= "</u>";
            unset(self::$vld["u"]);
        }
    }

    /**
     * Start striked-through text. Has to be implemented by writer plugins.
     */
    public static function strike_open()
    {
        if(!empty(self::$vld["s"]))
            self::report_error("strike-through is already open");
        else {
            if(!empty(self::$writer)) self::$writer->strike_open();
            else self::$out .= "<s>";
            self::$vld["s"] = 1;
        }
    }

    /**
     * Stop striked-through text. Has to be implemented by writer plugins.
     */
    public static function strike_close()
    {
        if(empty(self::$vld["s"]))
            self::report_error("cannot close, strike-through is not open");
        else {
            if(!empty(self::$writer)) self::$writer->strike_close();
            else self::$out .= "</s>";
            unset(self::$vld["s"]);
        }
    }

    /**
     * Start superscript text. Has to be implemented by writer plugins.
     */
    public static function superscript_open()
    {
        @self::$vld["sup"]++;
        if(!empty(self::$writer)) self::$writer->superscript_open();
        else self::$out .= "<sup>";
    }

    /**
     * Stop superscript text. Has to be implemented by writer plugins.
     */
    public static function superscript_close()
    {
        if(empty(self::$vld["sup"]))
            self::report_error("cannot close, superscript is not open");
        else {
            if(!empty(self::$writer)) self::$writer->superscript_close();
            else self::$out .= "</sup>";
            self::$vld["sup"]--;
        }
    }

    /**
     * Start subscript text. Has to be implemented by writer plugins.
     */
    public static function subscript_open()
    {
        @self::$vld["sub"]++;
        if(!empty(self::$writer)) self::$writer->subscript_open();
        else self::$out .= "<sub>";
    }

    /**
     * Stop subscript text. Has to be implemented by writer plugins.
     */
    public static function subscript_close()
    {
        if(empty(self::$vld["sub"]))
            self::report_error("cannot close, subscript is not open");
        else {
            if(!empty(self::$writer)) self::$writer->subscript_close();
            else self::$out .= "</sub>";
            self::$vld["sub"]--;
        }
    }

    /**
     * Start quote text. Has to be implemented by writer plugins.
     */
    public static function quote_open()
    {
        if(!empty(self::$vld["quote"]))
            self::report_error("cannot open, quote is already open");
        else {
            @self::$vld["quote"]++;
            if(!empty(self::$writer)) self::$writer->quote_open();
            else self::$out .= "<blockquote class=\"pre\"><span></span>";
        }
    }

    /**
     * Stop quote text. Has to be implemented by writer plugins.
     */
    public static function quote_close()
    {
        if(empty(self::$vld["quote"]))
            self::report_error("cannot close, quote is not open");
        else {
            if(!empty(self::$writer)) self::$writer->quote_close();
            else self::$out .= "</blockquote>";
            self::$vld["quote"]--;
        }
    }

    /**
     * Add line break. Has to be implemented by writer plugins.
     */
    public static function line_break()
    {
        if(!empty(self::$writer)) self::$writer->line_break();
        else self::$out .= "<br>";
    }

    /**
     * Add a horizontal ruler. Has to be implemented by writer plugins.
     */
    public static function horizontal_ruler()
    {
        if(!empty(self::$writer)) self::$writer->horizontal_ruler();
        else self::$out .= "<hr>";
    }

    /**
     * Start ordered list. Has to be implemented by writer plugins.
     */
    public static function ordered_list_open()
    {
        @self::$vld["ol"]++;
        if(!empty(self::$writer)) self::$writer->ordered_list_open();
        else self::$out .= "<ol>";
    }

    /**
     * End ordered list. Has to be implemented by writer plugins.
     */
    public static function ordered_list_close()
    {
        if(empty(self::$vld["ol"]))
            self::report_error("cannot close, ordered list is not open");
        else {
            if(@self::$vld["li"] >= self::$vld["ol"] + @self::$vld["ul"])
                self::report_error("list item is still open");
            if(!empty(self::$writer)) self::$writer->ordered_list_close();
            else self::$out .= "</ol>";
            self::$vld["ol"]--;
        }
    }

    /**
     * Start unordered list. Has to be implemented by writer plugins.
     */
    public static function unordered_list_open()
    {
        @self::$vld["ul"]++;
        if(!empty(self::$writer)) self::$writer->unordered_list_open();
        else self::$out .= "<ul>";
    }

    /**
     * End unordered list. Has to be implemented by writer plugins.
     */
    public static function unordered_list_close()
    {
        if(empty(self::$vld["ul"]))
            self::report_error("cannot close, unordered list is not open");
        else {
            if(@self::$vld["li"] >= self::$vld["ul"] + @self::$vld["ol"])
                self::report_error("list item is still open");
            if(!empty(self::$writer)) self::$writer->unordered_list_close();
            else self::$out .= "</ul>";
            self::$vld["ul"]--;
        }
    }

    /**
     * Start list item. Has to be implemented by writer plugins.
     */
    public static function list_item_open()
    {
        if(empty(self::$vld["ol"]) && empty(self::$vld["ul"]))
            self::report_error("cannot add list item, no list is open");
        else {
            @self::$vld["li"]++;
            if(!empty(self::$writer)) self::$writer->list_item_open();
            else self::$out .= "<li>";
        }
    }

    /**
     * End list item. Has to be implemented by writer plugins.
     */
    public static function list_item_close()
    {
        if(@self::$vld["li"] < @self::$vld["ol"] + @self::$vld["ul"])
            self::report_error("cannot close, list item is not open");
        else {
            if(!empty(self::$writer)) self::$writer->list_item_close();
            else self::$out .= "</li>";
            self::$vld["li"]--;
        }
    }

    /**
     * Start data list. Has to be implemented by writer plugins.
     */
    public static function data_list_open()
    {
        @self::$vld["dl"]++;
        if(!empty(self::$writer)) self::$writer->data_list_open();
        else self::$out .= "<dl>";
    }

    /**
     * End data list. Has to be implemented by writer plugins.
     */
    public static function data_list_close()
    {
        if(empty(self::$vld["dl"]))
            self::report_error("cannot close, data list is not open");
        else {
            if(@self::$vld["dt"] > self::$vld["dl"] || @self::$vld["dd"] > self::$vld["dl"])
                self::report_error("data list item is still open");
            if(!empty(self::$writer)) self::$writer->data_list_close();
            else self::$out .= "</dl>";
            self::$vld["dl"]--;
        }
    }

    /**
     * Start data topic. Has to be implemented by writer plugins.
     */
    public static function data_topic_open()
    {
        if(empty(self::$vld["dl"]))
            self::report_error("cannot add data topic, data list is not open");
        else {
            @self::$vld["dt"]++;
            if(!empty(self::$writer)) self::$writer->data_topic_open();
            else self::$out .= "<dt>";
        }
    }

    /**
     * End data topic. Has to be implemented by writer plugins.
     */
    public static function data_topic_close()
    {
        if(@self::$vld["dt"] < @self::$vld["dl"])
            self::report_error("cannot close, data list is not open");
        else {
            if(!empty(self::$writer)) self::$writer->data_topic_close();
            else self::$out .= "</dt>";
            self::$vld["dt"]--;
        }
    }

    /**
     * Start data description. Has to be implemented by writer plugins.
     */
    public static function data_description_open()
    {
        if(empty(self::$vld["dl"]))
            self::report_error("cannot add data description, data list is not open");
        else {
            @self::$vld["dd"]++;
            if(!empty(self::$writer)) self::$writer->data_description_open();
            else self::$out .= "<dd>";
        }
    }

    /**
     * End data description. Has to be implemented by writer plugins.
     */
    public static function data_description_close()
    {
        if(@self::$vld["dd"] < @self::$vld["dl"])
            self::report_error("cannot close, data description is not open");
        else {
            if(!empty(self::$writer)) self::$writer->data_description_close();
            else self::$out .= "</dd>";
            self::$vld["dd"]--;
        }
    }

    /**
     * Start grid. Has to be implemented by writer plugins.
     */
    public static function grid_open()
    {
        @self::$vld["grid"]++;
        if(!empty(self::$writer)) self::$writer->grid_open();
        else self::$out .= "<table class=\"grid\">";
    }

    /**
     * Start grid row. Has to be implemented by writer plugins.
     */
    public static function grid_row_open()
    {
        if(empty(self::$vld["grid"]))
            self::report_error("cannot add row, grid is not open");
        else {
            @self::$vld["gr"]++;
            if(!empty(self::$writer)) self::$writer->grid_row_open();
            else self::$out .= "<tr>";
        }
    }

    /**
     * Start grid cell. Has to be implemented by writer plugins.
     */
    public static function grid_cell_open()
    {
        if(empty(self::$vld["gr"]) || self::$vld["gr"] < @self::$vld["grid"])
            self::report_error("cannot add grid cell, grid row is not open");
        else {
            @self::$vld["gd"]++;
            if(!empty(self::$writer)) self::$writer->grid_cell_open();
            else self::$out .= "<td>";
        }
    }

    /**
     * Start wide grid cell. Has to be implemented by writer plugins.
     */
    public static function grid_cell_wide_open()
    {
        if(empty(self::$vld["gr"]) || self::$vld["gr"] < @self::$vld["grid"])
            self::report_error("cannot add grid cell, grid row is not open");
        else {
            @self::$vld["gd"]++;
            if(!empty(self::$writer)) self::$writer->grid_cell_wide_open();
            else self::$out .= "<td class=\"wide\">";
        }
    }

    /**
     * End grid cell. Has to be implemented by writer plugins.
     */
    public static function grid_cell_close()
    {
        if(empty(self::$vld["gd"]) || self::$vld["gd"] < @self::$vld["gr"])
            self::report_error("cannot close, grid cell is not open");
        else {
            if(!empty(self::$writer)) self::$writer->grid_cell_close();
            else self::$out .= "</td>";
            self::$vld["gd"]--;
        }
    }

    /**
     * End grid row. Has to be implemented by writer plugins.
     */
    public static function grid_row_close()
    {
        if(empty(self::$vld["gr"]) || self::$vld["gr"] < @self::$vld["grid"])
            self::report_error("cannot close, grid row is not open");
        else {
            if(@self::$vld["gd"] > self::$vld["gr"])
                self::report_error("grid cell is still open");
            if(!empty(self::$writer)) self::$writer->grid_row_close();
            else self::$out .= "</tr>";
            self::$vld["gr"]--;
        }
    }

    /**
     * End grid. Has to be implemented by writer plugins.
     */
    public static function grid_close()
    {
        if(empty(self::$vld["grid"]))
            self::report_error("cannot close, grid is not open");
        else {
            if(@self::$vld["gr"] > self::$vld["grid"])
                self::report_error("grid row is still open");
            if(!empty(self::$writer)) self::$writer->grid_close();
            else self::$out .= "</table>";
            self::$vld["grid"]--;
        }
    }

    /**
     * Start table. Has to be implemented by writer plugins.
     */
    public static function table_open()
    {
        @self::$vld["table"]++;
        if(!empty(self::$writer)) self::$writer->table_open();
        else self::$out .= "<div class=\"table\"><table>";
    }

    /**
     * Start table row. Has to be implemented by writer plugins.
     */
    public static function table_row_open()
    {
        if(empty(self::$vld["table"]))
            self::report_error("cannot add row, table is not open");
        else {
            @self::$vld["tr"]++;
            if(!empty(self::$writer)) self::$writer->table_row_open();
            else self::$out .= "<tr>";
        }
    }

    /**
     * Start table header. Has to be implemented by writer plugins.
     */
    public static function table_header_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add header, table row is not open");
        else {
            @self::$vld["th"]++;
            if(!empty(self::$writer)) self::$writer->table_header_open();
            else self::$out .= "<th>";
        }
    }

    /**
     * Start wide table header. Has to be implemented by writer plugins.
     */
    public static function table_header_wide_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add header, table row is not open");
        else {
            @self::$vld["th"]++;
            if(!empty(self::$writer)) self::$writer->table_header_wide_open();
            else self::$out .= "<th class=\"wide\">";
        }
    }

    /**
     * End table header. Has to be implemented by writer plugins.
     */
    public static function table_header_close()
    {
        if(empty(self::$vld["th"]) || self::$vld["th"] < @self::$vld["tr"])
            self::report_error("cannot close, table header is not open");
        else {
            if(!empty(self::$writer)) self::$writer->table_header_close();
            else self::$out .= "</th>";
            self::$vld["th"]--;
        }
    }

    /**
     * Start table cell. Has to be implemented by writer plugins.
     */
    public static function table_cell_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add cell, table row is not open");
        else {
            @self::$vld["td"]++;
            if(!empty(self::$writer)) self::$writer->table_cell_open();
            else self::$out .= "<td>";
        }
    }

    /**
     * Start wide table cell. Has to be implemented by writer plugins.
     */
    public static function table_cell_wide_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add cell, table row is not open");
        else {
            @self::$vld["td"]++;
            if(!empty(self::$writer)) self::$writer->table_cell_wide_open();
            else self::$out .= "<td class=\"wide\">";
        }
    }

    /**
     * Start numeric table cell. Has to be implemented by writer plugins.
     */
    public static function table_number_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add cell, table row is not open");
        else {
            @self::$vld["td"]++;
            if(!empty(self::$writer)) self::$writer->table_number_open();
            else self::$out .= "<td class=\"right\">";
        }
    }

    /**
     * Start wide numeric table cell. Has to be implemented by writer plugins.
     */
    public static function table_number_wide_open()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot add cell, table row is not open");
        else {
            @self::$vld["td"]++;
            if(!empty(self::$writer)) self::$writer->table_number_wide_open();
            else self::$out .= "<td class=\"right wide\">";
        }
    }

    /**
     * End table cell. Has to be implemented by writer plugins.
     */
    public static function table_cell_close()
    {
        if(empty(self::$vld["td"]) || self::$vld["td"] < @self::$vld["tr"])
            self::report_error("cannot close, table header is not open");
        else {
            if(!empty(self::$writer)) self::$writer->table_cell_close();
            else self::$out .= "</td>";
            self::$vld["td"]--;
        }
    }

    /**
     * End table row. Has to be implemented by writer plugins.
     */
    public static function table_row_close()
    {
        if(empty(self::$vld["tr"]) || self::$vld["tr"] < @self::$vld["table"])
            self::report_error("cannot close, table row is not open");
        else {
            if(@self::$vld["td"] > self::$vld["tr"])
                self::report_error("table cell is still open");
            if(!empty(self::$writer)) self::$writer->table_row_close();
            else self::$out .= "</tr>";
            self::$vld["tr"]--;
        }
    }

    /**
     * End table. Has to be implemented by writer plugins.
     */
    public static function table_close()
    {
        if(empty(self::$vld["table"]))
            self::report_error("cannot close, table is not open");
        else {
            if(@self::$vld["tr"] > self::$vld["table"])
                self::report_error("table row is still open");
            if(!empty(self::$writer)) self::$writer->table_close();
            else self::$out .= "</table></div>";
            self::$vld["table"]--;
        }
    }

    /**
     * Add teletype (monospace) text. Has to be implemented by writer plugins.
     * @param string the text
     */
    public static function teletype($str)
    {
        if(!empty(self::$writer)) self::$writer->teletype($str);
        else self::$out .= "<samp>".htmlspecialchars($str)."</samp>";
    }

    /**
     * Add preformatted (multiline monospace) text. Has to be implemented by writer plugins.
     * @param string the text
     */
    public static function preformatted($str)
    {
        if(!empty(self::$writer)) self::$writer->preformatted($str);
        else self::$out .= "<div class=\"pre\"><pre>".strtr(htmlspecialchars($str), [ "&lt;hl&gt;" => "<span class=\"hl_h\">",
            "&lt;/hl&gt;" => "</span>", "&lt;hm&gt;" => "<span class=\"hl_h hl_b\">", "&lt;/hm&gt;\r\n" => "</span>",
            "&lt;/hm&gt;\n" => "</span>", "&lt;/hm&gt;" => "</span>" ])."</pre></div>";
    }

    /**
     * Add source code. Has to be implemented by writer plugins.
     * @param string the source code as string
     * @param string the language type (or empty string)
     * @param array tokenized source code (for syntax highlighting)
     */
    public static function source_code($str, $type = "", $tokens = [])
    {
        /* load highlight rules */
        $r=[];
        if(!empty($type)) {
            if(!empty(self::$rules[$type])) $r = self::$rules[$type];
            else echo("gendoc warning: ".self::$fn.":".self::$l.": no highlight rules for '".$type."' using generics\r\n");
        }
        /* use a generic scheme */
        if(empty($r)) $r = [ [ "\/\/.*?$", "\/\*.*?\*\/", "#.*?$" ], [ ], [ "[:=\<\>\+\-\*\/%&\^\|!][:=]?" ],
            [ "[0-9][0-9bx]?[0-9\.a-f\+\-]*?" ], [ "\"", "\'", "`" ], [ "[", "]", "{", "}", ")", ",", ";" ],
            [ "char", "int", "float", "true", "false", "nil", "null", "nullptr", "none",
              "public", "static", "struct", "from", "with", "new", "delete" ],
            [ "import", "def", "if", "then", "else", "endif", "elif", "switch", "case", "loop", "until", "for", "foreach", "as",
              "is", "in", "or", "and", "while", "do", "break", "continue", "function", "return", "enum", "try", "catch",
              "volatile", "class", "typedef" ]
        ];
        $t = [];
        /* tokenize string */
        for($k = 0; $k < strlen($str); ) {
            if(!$k && substr($str, 0, 2) == "#!") {
                while($k < strlen($str) && $str[$k] != '\r' && $str[$k] != '\n') $k++;
                $t[] = [ "c", substr($str, 0, $k) ];
            }
            if($str[$k] == "(") { $t[] = [ "", "(" ]; $k++; }
            if(preg_match("/<[\/]?h[lm]>/Aims", $str, $m, 0, $k)) {
                if(empty($t) || $t[count($t) - 1][0] != "") $t[] = [ "", "" ];
                $t[count($t) - 1][1] .= $m[0]; $k += strlen($m[0]); continue; }
            if(in_array($str[$k], [ ')', ' ', "\t", "\r", "\n" ]) || in_array($str[$k], $r[5])) {
                if(empty($t) || $t[count($t) - 1][0] != "") $t[] = [ "", "" ];
                $t[count($t) - 1][1] .= $str[$k++]; continue;
            }
            foreach([ "c", "p", "o", "n" ] as $o => $O) if(!($O == "n" && !empty($t) && $t[count($t) - 1][0] == "v")) {
                foreach($r[$o] as $n) if(preg_match("/".$n."/Aims", $str, $m, 0, $k)) {
                    if(empty($t) || $t[count($t) - 1][0] != $O) $t[] = [ $O, "" ];
                    $t[count($t) - 1][1] .= $m[0]; $k += strlen($m[0]); continue 3; }
            }
            foreach($r[4] as $n) {
                if(substr($str, $k, strlen($n)) == $n) {
                    $d = $n[strlen($n) - 1];
                    for($n = $k + strlen($n); $n < strlen($str); $n++) {
                        if($str[$n] == "\\") $n++; else
                        if($str[$n] == $d) { if(@$str[$n + 1] != $d) break; else $n++; }
                    }
                    $t[] = [ "s", substr($str, $k, $n - $k + 1) ]; $k = $n + 1; continue 2;
                }
            }
            if(empty($t) || $t[count($t) - 1][0] != "v") $t[] = [ "v", "" ];
            $t[count($t) - 1][1] .= $str[$k++];
        }
        foreach($t as $k => $T) {
            if($T[0] == "v") {
                if(in_array(strtolower($T[1]), $r[6])) $t[$k][0] = "t"; else
                if(in_array(strtolower($T[1]), $r[7])) $t[$k][0] = "k"; else
                if((!empty($t[$k + 1]) && $t[$k + 1][1][0] == "(") || (!empty($t[$k + 2]) && $t[$k + 1][0] == "" && $t[$k + 2][1][0] == "("))
                    $t[$k][0] = "f";
            }
            if($T[0] == "n" && @$t[$k - 1][0] == "o" && ($t[$k - 1][1] == "-" || $t[$k - 1][1] == ".")) {
                $t[$k][1] = $t[$k - 1][1].$t[$k][1];
                unset($t[$k - 1]);
            }
        }
        if(!empty(self::$writer)) self::$writer->source_code($str, $type, $t);
        else {
            $L = substr_count(rtrim($str), "\n") + 1;
            self::$out .= "<div class=\"pre\"><pre class=\"lineno\">";
            for($k = 0; $k < $L; $k++) self::$out .= ($k + 1)."<br>";
            self::$out .= "</pre><code>";
            $c = "";
            /* concatenate tokens into a string with highlight spans */
            foreach($t as $k => $T) {
                if($T[0] == "") $c .= htmlspecialchars($T[1]);
                else $c .= "<span class=\"hl_".$T[0]."\">".htmlspecialchars($T[1])."</span>";
            }
            self::$out .= strtr($c, [ "&lt;hl&gt;" => "<span class=\"hl_h\">", "&lt;/hl&gt;" => "</span>",
                "&lt;hm&gt;" => "<span class=\"hl_h hl_b\">", "&lt;/hm&gt;\r\n" => "</span>", "&lt;/hm&gt;\n" => "</span>",
                "&lt;/hm&gt;" => "</span>" ])."</code></div>";
        }
    }

    /**
     * Add link to previous page. Could be implemented by writer plugins.
     */
    public static function prev_link()
    {
        if(!empty(self::$prev)) {
            if(!empty(self::$writer)) self::$writer->prev_link();
            else self::$out .= "<br style=\"clear:both;\"><label class=\"btn prev\" accesskey=\"p\" for=\"_".(self::$prev!="_"?self::$prev:"")."\"".(self::$prev!="_"?" title=\"".htmlspecialchars(self::$toc[self::$prev][1])."\"":"").">".self::$lang["prev"]."</label>";
        }
    }

    /**
     * Add link to next page. Could be implemented by writer plugins.
     * @param string heading label id
     * @param string heading user-readable name
     */
    public static function next_link($id, $name)
    {
        if(!empty(self::$writer)) self::$writer->next_link($id, $name);
        else {
            self::$out .= (!empty($id) ? (empty(self::$prev) ? "<br style=\"clear:both;\">" : "")."<label class=\"btn next\" accesskey=\"n\" for=\"_".$id."\" title=\"".htmlspecialchars($name)."\">".self::$lang["next"]."</label>" : "");
            self::$out .= "</div>\n";
        }
    }

    /**
     * Add an internal link to any heading. Could be implemented by writer plugins.
     * @param string heading user-readable name
     * @param string where to link to, could be a substitute string for forward references
     */
    public static function internal_link($name, $lnk = "")
    {
        if(empty($lnk)) $lnk = self::safeid($name);
        if(!empty($lnk)) {
            if(empty(self::$toc[$lnk])) {
                if(empty(self::$fwd[$lnk]))
                    self::$fwd[$lnk] = self::$fn.":".self::$l.": unresolved link: ".$name;
                $lnk = "@GENDOC:".$lnk."@";
            }
            if(!empty(self::$writer)) self::$writer->internal_link($name, $lnk);
            /* we use onclick too because some browsers do not reload the page if only the anchor changes */
            else self::$out .= "<a href=\"#".$lnk."\" onclick=\"c('".$lnk."')\">".$name."</a>";
        }
    }

    /**
     * Resolve a forward internal link. Could be implemented by writer plugins.
     * @param string the substitution string that was used as id
     * @param string the real id
     */
    public static function resolve_link($subst, $id)
    {
        unset(self::$fwd[$subst]);
        if(!empty(self::$writer)) self::$writer->resolve_link("@GENDOC:".$subst."@", $id);
        else self::$out = str_replace("@GENDOC:".$subst."@", $id, self::$out);
    }

    /**
     * Start an external link. Could be implemented by writer plugins.
     * @param string link's URL
     */
    public static function external_link_open($url)
    {
        /* if url starts with '#', then insert it as an internal link, but without checks */
        if(!empty(self::$writer)) self::$writer->external_link_open($url);
        else self::$out .= "<a href=\"".$url."\" ".($url[0] == '#' ? "onclick=\"c('".substr($url,1)."')\"" : "target=\"new\"").">";
    }

    /**
     * End an external link. Could be implemented by writer plugins.
     */
    public static function external_link_close()
    {
        if(!empty(self::$writer)) self::$writer->external_link_close();
        else self::$out .= "</a>";
    }

    /**
     * Start a user interface element. Has to be implemented by writer plugins.
     * @param string type, "1" to "6"
     */
    public static function user_interface_open($type)
    {
        if(!empty(self::$vld["ui"]))
            self::report_error("user interface element is already open");
        else {
            if(!empty(self::$writer)) self::$writer->user_interface_open($type);
            else self::$out .= "<span class=\"ui".$type."\">";
            @self::$vld["ui"]++;
        }
    }

    /**
     * End a user interface element. Has to be implemented by writer plugins.
     */
    public static function user_interface_close()
    {
        if(empty(self::$vld["ui"]))
            self::report_error("cannot close, user interface element is not open");
        else {
            if(!empty(self::$writer)) self::$writer->user_interface_close();
            else self::$out .= "</span>";
            self::$vld["ui"]--;
        }
    }

    /**
     * Add a keyboard key. Has to be implemented by writer plugins.
     * @param string key's name
     */
    public static function keyboard($key)
    {
        if(!empty(self::$writer)) self::$writer->keyboard($key);
        else self::$out .= "<kbd>".htmlspecialchars($key)."</kbd>";
    }

    /**
     * Add a mouse button image. Has to be implemented by writer plugins.
     * @param string either "l" (left button), "r" (right button) or "w" (wheel)
     */
    public static function mouse_button($type)
    {
        if($type != "r" && $type != "w") $type = "l";
        if(!empty(self::$writer)) self::$writer->mouse_button($type);
        else self::$out .= "<span class=\"mouse".($type == "r" ? "right" : ($type == "w" ? "wheel" : "left"))."\"></span>";
    }

    /**
     * Add an image. Has to be implemented by writer plugins.
     * @param string either "t" (inlined as text), "l" (left), "r" (right), "c" (centered) or "w" (wide, centered)
     * @param string image's file name
     * @param array index 0 width, 1 height, 'mime' mime-type
     * @param string image data
     */
    public static function image($align, $fn, $a = [], $img = "")
    {
        if($align != "l" && $align != "r" && $align != "c" && $align != "w") $align = "t";
        $l = strlen(self::$lang["url"]);
        $imfn = dirname(self::$fn)."/".(substr($fn,0,$l) == self::$lang["url"] ? substr($fn, $l) : $fn);
        $img = @file_get_contents($imfn);
        $a = @getimagesizefromstring($img);
        if($align == "t" && function_exists("imagecreatefromstring") && ($i = @imagecreatefromstring($img))) {
            $X = imagesx($i); $Y = imagesy($i);
            if($Y > 22) {
                $a[0] = floor(22*$X/$Y); $a[1] = 22;
                $N = imagecreatetruecolor($a[0], 22);
                imagealphablending($N, 0);imagesavealpha($N, 1);imagefill($N, 0, 0, imagecolorallocatealpha($N, 0, 0, 0, 0));
                imagecopyresampled($N, $i, 0, 0, 0, 0, $a[0], $a[1], $X, $Y);
                ob_start();imagepng($N);$d=ob_get_contents();ob_end_clean();
                imagedestroy($N);
                if(strlen($d) < strlen($img)) { $img = $d; $a['mime'] = "image/png"; }
            }
            imagedestroy($i);
        }
        if(empty($img) || @$a[0] < 1 || @$a[1] < 1 || empty($a['mime'])) {
            self::report_error("unable to read image '".$imfn."'");
        } else {
            if(!empty(self::$writer)) self::$writer->image($align, $fn, $a, $img);
            else self::$out .= ($align == "c" ? "<div class=\"imgc\">" : "").
                "<img class=\"img".$align."\"".($align != "w" ? " width=\"".$a[0]."\" height=\"".$a[1]."\"" : "").
                " alt=\"".htmlspecialchars(basename($fn))."\" src=\"data:".$a['mime'].";base64,".base64_encode($img)."\">".
                ($align == "c" ? "</div>" : "");
        }
    }

    /**
     * Add an image caption. Has to be implemented by writer plugins.
     * @param string figure description
     */
    public static function figure($name)
    {
        if(!empty(self::$writer)) self::$writer->figure($name);
        else self::$out .= "<div class=\"fig\">".$name."</div>";
    }

    /**
     * Start an alert box. Has to be implemented by writer plugins.
     * @param string either "info", "hint", "note", "also", "todo" or "warn"
     */
    public static function alert_box_open($type)
    {
        if(!in_array($type, [ "hint", "note", "also", "todo", "warn" ])) $type = "info";
        if(!empty(self::$vld["alert"]))
            self::report_error("cannot open, alert box is already open");
        else {
            if(!empty(self::$writer)) self::$writer->alert_box_open($type);
            else self::$out .= "<div class=\"".($type == "hint" ? "hint" : ($type == "todo" || $type == "warn" ? "warn" : "info"))."\">".
                "<p><span>".self::$lang[$type]."</span></p><p>";
            @self::$vld["alert"]++;
        }
    }

    /**
     * End an alert box. Has to be implemented by writer plugins.
     */
    public static function alert_box_close()
    {
        if(empty(self::$vld["alert"]))
            self::report_error("cannot close, alert box is not open");
        else {
            if(!empty(self::$writer)) self::$writer->alert_box_close();
            self::$out .= "</p></div>";
            self::$vld["alert"]--;
        }
    }

    /**
     * Add any arbitrary text to output. If a tag is open, then to that. Has to be implemented by writer plugins.
     * @param string the text
     */
    public static function text($str)
    {
        if(!empty(self::$writer)) self::$writer->text($str);
        else self::$out .= $str;
    }

    /**
     * Parse a source document. Has to be implemented by file format reader plugins.
     * @param string source document's content
     * @param gendoc::\$fn name of the source file
     * @return gendoc::\$l current line number, must be incremented by the reader plugin
     */
    public static function parse($s)
    {
        for($i = 0; $i < strlen($s); ) {
            /* skip comments */
            if(substr($s, $i, 4) == "<!--") {
                for($i += 4; $i < strlen($s) - 3 && substr($s, $i, 3) != "-->"; $i++) {
                    if($s[$i] == "\n") self::$l++;
                }
                $i += 3; continue;
            }
            /* copy everything between tags */
            if($s[$i] == "\n") self::$l++;
            if($s[$i] != '<') {
                if($s[$i] == ' ' || $s[$i] == "\t") {
                    if($i == 0 || ($s[$i - 1] != ' ' && $s[$i - 1] != "\t" && $s[$i - 1] != "\n")) self::text(" ");
                    $i++; continue;
                }
                if($s[$i] == "\r" || ($s[$i] == "\n" && !empty(self::$out) && self::$out[strlen(self::$out) - 1] == "\n")) { $i++; continue; }
                self::text($s[$i++]); continue;
            }

            /* our <doc> tag, defining some variables. Skip that from output */
            if(substr($s, $i, 5) == "<doc>") {
                $j = $i + 5; $i = strpos($s, "</doc>", $i) + 6;
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                self::doc(substr($s, $j, $i - $j));
            } else
            /* hello page markers */
            if(substr($s, $i, 7) == "<hello>") { $i += 7; self::hello_open(); } else
            if(substr($s, $i, 8) == "</hello>") { $i += 8; self::hello_close(); } else
            /* parse headings and collect Table of Contents info */
            if(substr($s, $i, 2) == "<h" && ord($s[$i + 2]) >= 0x31 && ord($s[$i + 2]) <= 0x36) {
                $j = $i + 3; $i = strpos($s, "</h", $i + 2); $L = $s[$j - 1];
                $a = explode(">", substr($s, $j, $i - $j));
                if(!empty($a[0])) {
                    $b = explode(" ", trim($a[0]));
                    $id = !empty($b[1]) ? $b[1] : $b[0];
                    $alias = !empty($b[1]) ? $b[0] : "";
                } else {
                    $id = $a[1];
                    $alias = "";
                }
                self::heading($L, $a[1], $id, $alias);
                self::$l += substr_count($a[1], "\n");
                $i += 5;
            } else
            /* Table of Contents caption, just adds to the toc, but does not generate output */
            if(substr($s, $i, 5) == "<cap>") {
                $j = $i + 5; $i = strpos($s, "</cap>", $i);
                self::caption(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 6;
            } else
            /* styling text */
            if(substr($s, $i, 3) == "<b>") { $i += 3; self::bold_open(); } else
            if(substr($s, $i, 4) == "</b>") { $i += 4; self::bold_close(); } else
            if(substr($s, $i, 3) == "<i>") { $i += 3; self::italic_open(); } else
            if(substr($s, $i, 4) == "</i>") { $i += 4; self::italic_close(); } else
            if(substr($s, $i, 3) == "<u>") { $i += 3; self::underline_open(); } else
            if(substr($s, $i, 4) == "</u>") { $i += 4; self::underline_close(); } else
            if(substr($s, $i, 3) == "<s>") { $i += 3; self::strike_open(); } else
            if(substr($s, $i, 4) == "</s>") { $i += 4; self::strike_close(); } else
            if(substr($s, $i, 5) == "<sup>") { $i += 5; self::superscript_open(); } else
            if(substr($s, $i, 6) == "</sup>") { $i += 6; self::superscript_close(); } else
            if(substr($s, $i, 5) == "<sub>") { $i += 5; self::subscript_open(); } else
            if(substr($s, $i, 6) == "</sub>") { $i += 6; self::subscript_close(); } else
            if(substr($s, $i, 7) == "<quote>") { $i += 7; self::quote_open(); } else
            if(substr($s, $i, 8) == "</quote>") { $i += 8; self::quote_close(); } else
            /* structuring text */
            if(substr($s, $i, 3) == "<p>") { $i += 3; self::paragraph_open(); } else
            if(substr($s, $i, 4) == "</p>") { $i += 4; self::paragraph_close(); } else
            /* line breaks */
            if(substr($s, $i, 4) == "<br>") { $i += 4; self::line_break(); } else
            if(substr($s, $i, 4) == "<hr>") { $i += 4; self::horizontal_ruler(); } else
            /* lists */
            if(substr($s, $i, 4) == "<ol>") { $i += 4; self::ordered_list_open(); } else
            if(substr($s, $i, 5) == "</ol>") { $i += 5; self::ordered_list_close(); } else
            if(substr($s, $i, 4) == "<ul>") { $i += 4; self::unordered_list_open(); } else
            if(substr($s, $i, 5) == "</ul>") { $i += 5; self::unordered_list_close(); } else
            if(substr($s, $i, 4) == "<li>") { $i += 4; self::list_item_open(); } else
            if(substr($s, $i, 5) == "</li>") { $i += 5; self::list_item_close(); } else
            /* data fields */
            if(substr($s, $i, 4) == "<dl>") { $i += 4; self::data_list_open(); } else
            if(substr($s, $i, 5) == "</dl>") { $i += 5; self::data_list_close(); } else
            if(substr($s, $i, 4) == "<dt>") { $i += 4; self::data_topic_open(); } else
            if(substr($s, $i, 5) == "</dt>") { $i += 5; self::data_topic_close(); } else
            if(substr($s, $i, 4) == "<dd>") { $i += 4; self::data_description_open(); } else
            if(substr($s, $i, 5) == "</dd>") { $i += 5; self::data_description_close(); } else
            /* grid, invisible table */
            if(substr($s, $i, 6) == "<grid>") { $i += 6; self::grid_open(); } else
            if(substr($s, $i, 7) == "</grid>") { $i += 7; self::grid_close(); } else
            if(substr($s, $i, 4) == "<gr>") { $i += 4; self::grid_row_open(); } else
            if(substr($s, $i, 5) == "</gr>") { $i += 5; self::grid_row_close(); } else
            if(substr($s, $i, 4) == "<gd>") { $i += 4; self::grid_cell_open(); } else
            if(substr($s, $i, 4) == "<gD>") { $i += 4; self::grid_cell_wide_open(); } else
            if(substr($s, $i, 5) == "</gd>") { $i += 5; self::grid_cell_close(); } else
            /* table, needs a surrounding div for scrollbars */
            if(substr($s, $i, 7) == "<table>") { $i += 7; self::table_open(); } else
            if(substr($s, $i, 8) == "</table>") { $i += 8; self::table_close(); } else
            if(substr($s, $i, 4) == "<tr>") { $i += 4; self::table_row_open(); } else
            if(substr($s, $i, 5) == "</tr>") { $i += 5; self::table_row_close(); } else
            /* table cell header */
            if(substr($s, $i, 4) == "<th>") { $i += 4; self::table_header_open(); } else
            if(substr($s, $i, 4) == "<tH>") { $i += 4; self::table_header_wide_open(); } else
            if(substr($s, $i, 5) == "</th>") { $i += 5; self::table_header_close(); } else
            /* table cell data */
            if(substr($s, $i, 4) == "<td>") { $i += 4; self::table_cell_open(); } else
            if(substr($s, $i, 4) == "<tD>") { $i += 4; self::table_cell_wide_open(); } else
            if(substr($s, $i, 5) == "</td>") { $i += 5; self::table_cell_close(); } else
            /* table cell aligned right (number) */
            if(substr($s, $i, 4) == "<tn>") { $i += 4; self::table_number_open(); } else
            if(substr($s, $i, 4) == "<tN>") { $i += 4; self::table_number_wide_open(); } else
            if(substr($s, $i, 5) == "</tn>") { $i += 5; self::table_cell_close(); } else
            /* internal link */
            if(substr($s, $i, 3) == "<a>") {
                $j = $i + 3; $i = strpos($s, "</a>", $i);
                self::internal_link(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 4;
            } else
            /* external link, open in a new tab */
            if(substr($s, $i, 3) == "<a ") {
                $i += 3; $j = $i; while($s[$i] != '>') $i++;
                self::external_link_open(substr($s, $j, $i - $j));
                $i++;
            } else
            if(substr($s, $i, 4) == "</a>") {
                self::external_link_close();
                $i += 4;
            } else
            /* add teletype tags to output without parsing */
            if(substr($s, $i, 4) == "<tt>") {
                $j = $i + 4; $i = strpos($s, "</tt>", $i);
                self::teletype(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 5;
            } else
            /* pre is pretty much the same, but adds an optional div around for scrollbars */
            if(substr($s, $i, 5) == "<pre>") {
                $j = $i + 5; $i = strpos($s, "</pre>", $i);
                self::preformatted(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 6;
            } else
            /* code is similar to pre, but needs line numbers and a syntax highlighter */
            if(substr($s, $i, 5) == "<code") {
                $i += 5; $j = $i; while($s[$j] == ' ') $j++;
                while($s[$i - 1] != '>') $i++;
                $c = $s[$j] != '>' ? substr($s, $j, $i - $j - 1) : "";
                while($s[$i] == "\n" || $s[$i] == "\r") { if($s[$i] == "\n") self::$l++; $i++; }
                $j = $i; $i = strpos($s, "</code>", $i);
                self::source_code(rtrim(substr($s, $j, $i - $j)), $c);
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 7;
            } else
            /* user interface elements */
            if(substr($s, $i, 3) == "<ui" && $s[$i + 3] >= '1' && $s[$i + 3] <= '6' && $s[$i + 4] == '>') {
                self::user_interface_open($s[$i + 3]);
                $i += 5;
            } else
            if(substr($s, $i, 4) == "</ui" && $s[$i + 4] >= '1' && $s[$i + 4] <= '6' && $s[$i + 5] == '>') {
                self::user_interface_close();
                $i += 6;
            } else
            /* keyboard keys */
            if(substr($s, $i, 5) == "<kbd>") {
                $j = $i + 5; $i = strpos($s, "</kbd>", $i);
                self::keyboard(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 6;
            } else
            /* mouse button left, right and wheel "icons" */
            if(substr($s, $i, 5) == "<mbl>") { $i += 5; self::mouse_button("l"); } else
            if(substr($s, $i, 5) == "<mbr>") { $i += 5; self::mouse_button("r"); } else
            if(substr($s, $i, 5) == "<mbw>") { $i += 5; self::mouse_button("w"); } else
            /* parse our image tags (<imgt>, <imgl>, <imgr>, <imgc>, <imgw>), but not the original HTML <img> tags */
            if(substr($s, $i, 4) == "<img" && $s[$i + 4] != ' ' && $s[$i + 4] != '/' && $s[$i + 4] != '>') {
                $i += 6; $j = $i; while($s[$i] != '>') $i++;
                self::image($s[$j - 2], explode("\n",trim(substr($s, $j, $i - $j)))[0]);
                $i++;
            } else
            /* image caption */
            if(substr($s, $i, 5) == "<fig>") {
                $j = $i + 5; $i = strpos($s, "</fig>", $i);
                self::figure(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
                $i += 6;
            } else
            /* alert boxes */
            if(in_array(substr($s, $i, 6),[ "<info>", "<hint>", "<note>", "<also>", "<todo>", "<warn>" ])) {
                self::alert_box_open(substr($s, $i + 1, 4));
                $i += 6;
            } else
            if(in_array(substr($s, $i, 7),[ "</info>", "</hint>", "</note>", "</also>", "</todo>", "</warn>" ])) {
                self::alert_box_close();
                $i += 7;
            } else
            /* include another input file */
            if(substr($s, $i, 9) == "<include ") {
                $i += 9; $j = $i; while($s[$i] != '>') $i++;
                self::include(substr($s, $j, $i - $j));
                $i++;
            } else
            /* generate api documentation */
            if(substr($s, $i, 5) == "<api ") {
                $i += 5; $j = $i; while($s[$i] != ' ') $i++;
                $c = substr($s, $j, $i - $j);
                $j = $i + 1; while($s[$i] != '>') $i++;
                self::api($c, substr($s, $j, $i - $j));
                $i++;
            } else {
                /* copy all the other tags to the output untouched */
                $j = $i; $i++; while($i < strlen($s) && $s[$i] != '<' && $s[$i - 1] != '>') $i++;
                echo("gendoc warning: ".self::$fn.":".self::$l.": not gendoc compatible tag '".explode("\n",trim(substr($s, $j)))[0]."'\r\n");
                self::text(substr($s, $j, $i - $j));
                self::$l += substr_count(substr($s, $j, $i - $j), "\n");
            }
        }
    }

    /**
     * Output documentation. Has to be implemented by writer plugins.
     * @param string name of the file to write to
     */
    public static function output($fn)
    {
        if(empty(self::$toc)) { echo("gendoc error: no table of contents detected\r\n"); self::$err++; }
        if(!empty(self::$fwd)) {
            foreach(self::$fwd as $v)
                echo("gendoc error: ".$v."\r\n");
            self::$err++;
        }
        if(!empty(self::$vld)) {
            foreach(self::$vld as $k => $v)
                if($v != 0)
                    self::report_error("unclosed ".$k." in section ".(empty(self::$lsec) ? "unknown" : self::$lsec));
            self::$vld = [];
        }
        self::$fn = $fn;
        self::$l = 0;
        if(!empty(self::$writer)) {
            self::$writer->prev_link();
            self::$writer->output($fn);
        } else {
            self::$out = trim(self::$out);
            if(!empty(self::$out)) {
                self::prev_link();
                self::$out .= "</div>";
            }
            /* load user specified theme */
            $theme = "hr,table,th,td{border-color:#e1e4e5;}\n".
                "th{background:#d6d6d6;}\n".
                "tr:nth-child(odd){background:#f3f6f6;}\n".
                "a{text-decoration:none;color:#2980B9;}\n".
                ".content{background:#fcfcfc;color:#404040;font-family:Lato,Helvetica,Neue,Arial,Deja Vu,sans-serif;}\n".
                ".title,.home,h1>a,h2>a,h3>a,h4>a,h5>a,h6>a{background:#2980B9;color:#fcfcfc;}\n".
                ".version{color:rgba(255,255,255,0.3);}\n".
                ".search{border:1px solid #2472a4;background:#fcfcfc;}\n".
                ".nav{background:#343131;color:#d9d9d9;}\n".
                ".nav p{color:#55a5d9;}\n".
                ".nav label:hover,.nav a:hover{background:#4e4a4a;}\n".
                ".nav .current{background:#fcfcfc;color:#404040;}\n".
                ".nav li>ul>li{background:#e3e3e3;}\n".
                ".nav li>ul>li>a{color:#404040;}\n".
                ".nav li>ul>li>a:hover{background:#d6d6d6;}\n".
                ".pre {border:1px solid #e1e4e5;background:#f8f8f8;}\n".
                ".info{background:#e7f2fa;}\n".
                ".info>p:first-child{background:#6ab0de;color:#fff;}\n".
                ".hint{background:#dbfaf4;}\n".
                ".hint>p:first-child{background:#1abc9c;color:#fff;}\n".
                ".warn{background:#ffedcc;}\n".
                ".warn>p:first-child{background:#f0b37e;color:#fff;}\n".
                ".btn{background:#f3f6f6;}\n".
                ".btn:hover{background:#e5ebeb;}\n".
                ".hl_h{background-color:#ccffcc;}\n".
                ".hl_c{color:#808080;font-style:italic;}\n".
                ".hl_p{color:#1f7199;}\n".
                ".hl_o{color:#404040;}\n".
                ".hl_n{color:#0164eb;}\n".
                ".hl_s{color:#986801;}\n".
                ".hl_t{color:#60A050;}\n".
                ".hl_k{color:#a626a4;}\n".
                ".hl_f{color:#2a9292;}\n".
                ".hl_v{color:#e95649;}\n";
            if(!empty(self::$lang["theme"])) {
                $theme = @file_get_contents(self::$lang["theme"]);
                if(empty($theme)) { echo("gendoc error: ".self::$lang["theme"].":0: unable to read theme css\r\n"); self::$err++; }
            }
            /* load title image */
            $titimg = ""; $title = self::$lang["title"];
            if(!empty(self::$lang["titleimg"])) {
                $s = self::$lang["titleimg"]; for($i = 0; $i < strlen($s) && $s[$i] != ' '; $i++);
                $title = trim(trim(substr($s, $i + 1)." ".self::$lang["title"]));
                $imfn = substr($s, 0, $i);
                $img = @file_get_contents($imfn);
                $a = @getimagesizefromstring($img);
                if(empty($img) || @$a[0] < 1 || @$a[1] < 1 || empty($a['mime'])) {
                    self::report_error("unable to read image '".$imfn."'");
                } else
                    $titimg = "<img alt=\"".htmlspecialchars(substr($s, $i + 1))."\" src=\"data:".$a['mime'].";base64,".base64_encode($img)."\">";
            }
            if(empty($title)) $title = "No Name";

            /*** output html ***/
            $f = fopen($fn, "wb+");
            if(!$f) die("gendoc error: ".$fn.":0: unable to write file\r\n");
            fwrite($f, "<!DOCTYPE html>\n<html lang=\"".self::$lang["lang"]."\">\n<head>\n  <meta charset=\"utf-8\">\n".
            "  <meta name=\"generator\" content=\"gendoc ".self::$version.": https://gitlab.com/bztsrc/gendoc\">\n".
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n".
            "  <title>".$title."</title>\n".
            /* embedded stylesheet licensed under CC-BY */
            "  <style rel=\"logic\">*{box-sizing:border-box;font-family:inherit;}".
            "body {background:rgba(0,0,0,0.05);font-weight:400;font-size:16px;}".
            "hr{display:block;height:1px;border:0;border-top:1px solid #e1e4e5;margin:26px 0;padding:0;border-top:1px solid;}".
            "br:after,br:before{display:table;content:\"\"}br{clear:both;}".
            "h1,h2,h3,h4,h5,h6{clear:both;margin:0px 0px 20px 0px;padding-top:4px;-webkit-font-smoothing:antialiased;-moz-osx-font-smoothing:grayscale;}".
            "p{margin:0 0 24px}a{cursor:pointer;}".
            "h1{font-size:175%}h2{font-size:150%}h3{font-size:125%}h4{font-size:115%}h5{font-size:110%}h6{font-size:100%}".
            "pre,samp,code,var,kbd{font-family:Monaco,Consolas,Liberation Mono,Courier,monospace;font-variant-ligatures:none;}".
            "pre,code{display:block;overflow:auto;white-space:pre;font-size:14px;line-height:16px!important;}pre{padding:12px;margin:0px;}".
            "code{padding:0 0 12px 0;margin:12px 12px 0px 2px;background:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAAgCAYAAADT5RIaAAAAFklEQVQI12NgYGDgZWJgYGCgDkFtAAAWnAAsyj4TxgAAAABJRU5ErkJggg==) 0 0 repeat;}".
            ".lineno{display:block;padding:0px 4px 0px 4px;margin:12px 0px 0px 0px;opacity:.4;text-align:right;float:left;white-space:pre;font-size:12px;line-height:16px!important;}".
            "pre .hl_b,samp .hl_b,code .hl_b{display:block;}".
            "blockquote{margin:0px;padding:12px;}blockquote>span:first-child::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAgCAYAAABU1PscAAABRElEQVRYw+3WTytEURjH8c/4l1JTpKxsyEbYWFkpG+UVKOWFeAts7eytbJSysLLVxIKNFAslwhSlUQabUdM0xm0e967Ot+7inPN8z+13z73nXBKJRCIRoJSxbggrmMMInlHBIWoF+KEAQ9jAaJuxe2zhJUe/Iz0ZahZ/uTmMYT1nPxxg/I/xWQzn6IcD1IIha//wkEIBKhlq+nP0wwHOsYuvDjWvOfod6c1Yd9O4ZjDQZvwAbzn6oRVofpKbqLf0P+GxAD8cAO7w0NJ3WqAfDlBCuan9gaMC/XCA+cbJ+sMRqgX6oQCTWGtqX2O/QL8tfRlqyljGUlPgW2y3+SDz8Lv6mRvEQmPbm25ZqQvs/LHtRf3wCkxgtaWvij2cZJg36ocD/Pw91nGJY5zhM+O8UT/8Ck01TswrvHcxb9RPJBKJRDF8AyNbWk4WFTIzAAAAAElFTkSuQmCC);float:left;vertical-align:top;}".
            ".ui1,.ui2,.ui3,.ui4,.ui5,.ui6{display:inline-block;height:24px!important;line-height:24px!important;padding:0px 4px;margin:-2px 0px -2px;}".
            "kbd{display:inline-block;font-weight:700;border:1px solid #888;height:24px!important;padding:0px 4px;margin:-2px 0px -2px;border-radius:4px;background-image:linear-gradient(#ddd 0%,#eee 10%,#bbb 10%,#ccc 30%,#fff 85%,#eee 85%,#888 100%);}".
            ".mouseleft,.mouseright,.mousewheel{display:inline-block;min-width:16px;height:24px!important;padding:0px;margin:-2px 0px 0px 0px;vertical-align:middle;}".
            ".mouseleft::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACoqKj9/f2zs7O1tbWRkZGlpaWsrKybm5u2traNjY2Wlpanp6empqaYmJiPj4+3t7f5+fmjo6P19fXu7u6ZmZnx8fHi4uLm5ube3t7q6uqwsLC0tLS7u7u4uLi/v7/W1tbDw8PLy8vPz8/T09Pa2trHx8eIiIhERkShhqFGAAAAAXRSTlMAQObYZgAAAI5JREFUGNNV0EcCwjAMRFEB6R2DKSGQUBLp/idEjsG23m7+cgAMIsJWwR8Z235pumDTnkUbv+lg7CIfTqvSbTquxsGFfjWXLlwsdOFs+XC1EHAWOEwCh4/A4S1weAkcFoHDU0CI8zGQx1Cpe0BVAO0j0PIfiR4cnZjLdHP7abQ9VRVZnaZ1VvjfO42o7edfH3EoHZS6XE4AAAAASUVORK5CYII=);}".
            ".mouseright::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACoqKj9/f2zs7O1tbWRkZGlpaWsrKybm5u2traNjY2Wlpanp6empqaYmJiPj4+3t7f5+fmjo6P19fXu7u6ZmZnx8fHi4uLm5ube3t7q6uqwsLC0tLS7u7u4uLi/v7/W1tbDw8PLy8vPz8/T09Pa2trHx8eIiIhERkShhqFGAAAAAXRSTlMAQObYZgAAAI9JREFUGNNV0NkWgjAMRdGozFOxWgdEcYLk/7/Q0EpL9tNd5/ECzLRCIoJF20zdlsinTbRn5Eu0O8zIl/Jk+dAPR4uWUo6d5QNenBDOTghXJ4RRQMCnwOErcPgIHN4Ch0ng8BIQ4nxYyWOo9H1FVwDqsaL4j8T0nknmy0xz+2uMO1UXWZ2mdVbo8LtBNK2dP/2+KB2shyfVAAAAAElFTkSuQmCC);}".
            ".mousewheel::before{content:url(data:type/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAYCAMAAADEfo0+AAAAe1BMVEUAAACzs7OoqKisrKyRkZGlpaX9/f22trabm5uNjY2mpqanp6ePj4+1tbWYmJi3t7eWlpajo6OZmZmwsLD5+fnu7u7x8fHi4uLW1tbm5ube3t7T09Pq6ur19fW4uLi7u7u0tLTa2trPz8+/v7/Dw8PLy8vHx8eIiIhERkS4354xAAAAAXRSTlMAQObYZgAAAKdJREFUGNNV0NkagiAUhdHjPAECzWWlWdL7P2Fno/nJumHzXx4iMMI5YeivVVOX592k2vkfy/1CxvjL6L6KJAd9ZF+GVxP144Eh4B170kPHEPAOmtwFEPxw5E6A4AeHKyD4wWEABD84nAHBDw43QPCDwyvA4RPgMAU4vAOO0mLcKFJqzHPDNETisSH4HpntVzbDyazaLZSdj2qqsk6Suqw2d7fO2fnmP7kAJW9a/HbiAAAAAElFTkSuQmCC);}".
            "footer{width:100%;padding:0 3.236em;}footer p{opacity:0.6;}footer small{opacity:0.5;}footer a{text-decoration:none;color:inherit;}footer a:hover{text-decoration:underline;}".
            "dl{margin:0 0 24px 0;padding:0px;}dt{font-weight:700;margin-bottom:12px;}dd{margin:0 0 12px 24px;}".
            ".table table{margin:0px;border-collapse:collapse;border-spacing:0;empty-cells:show;border:1px solid;width:100%;}".
            "th{font-weight:700;padding:8px 16px;overflow:visible;vertical-align:middle;white-space:nowrap;border:1px solid;}th.wide{width:100%;}".
            "td{padding:8px 16px;overflow:visible;vertical-align:middle;font-size:90%;border:1px solid;}td.right{text-align:right;}".
            "table.grid{margin:0px;padding:0px;border:none!important;background:none!important;border-spacing:0;border:0px!important;empty-cells:show;width:100%;}".
            "table.grid tr, table.grid td{margin:0px;padding:0px;overflow:hidden;vertical-align:top;background:none!important;border:0px!important;font-size:90%;}".
            "div.frame{position:absolute;width:100%;min-height:100%;margin:0px;padding:0px;max-width:1100px;top:0px;left:0px;}".
            "#_m{margin-left:300px;min-height:100%;}".
            "div.title{display:block;width:300px;padding-top:.809em;padding-bottom:.809em;margin-bottom:.809em;text-align:center;font-weight:700;}".
            "div.title>a{padding:4px 6px;margin-bottom:.809em;font-size:150%;}div.title>a:hover{background:transparent;}".
            "div.title>a>img{max-width:280px;border:0px;padding:0px;margin:0px;}".
            "div.title input{display:none;width:270px;border-radius:50px;padding:6px 12px;font-size:80%;box-shadow:inset 0 1px 3px #ddd;transition:border .3s linear;}".
            "div.title input:required:invalid{background:#fcfcfc url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAASFBMVEUAAAC6urq7u7y7vLy8vL3Dw8TLzM67urq8vL29vr69vr/AwMK5ubm5ubm4uLi5t7fAwMC/vr/FxMXBwsPExcW+u7vDw8S4uLiryZHFAAAAF3RSTlMA9vGttk4lfbWnpJQG0MpzYEIglFRCGzMa+EsAAAB0SURBVBjTbY5bCsMwDAQl2U6cR5s0fcz9b1oLpWBC52dhENqVRsmG5SIn60wwryEm0LQkbUacAveh5XGDh4uKfsQZlOqpJAkS5gHPUyzgYdeLjB6/H7lvGbzlssP2WDoRGGzxrVRD82sHRukZ/xhv7tns/QXrhgcaFdOKBwAAAABJRU5ErkJggg==) no-repeat 10px 50%;}".
            "div.title input:focus{background:#fcfcfc!important;}".
            "div.version{margin-top:.4045em;margin-bottom:.809em;font-size:90%;}".
            "nav.side {display:block;position:fixed;top:0;bottom:0;left:0;width:300px;overflow-x:hidden;overflow-y:hidden;min-height:100%;font-weight:400;z-index:999;}".
            "nav.mobile {display:none;font-weight:bold;padding:.4045em .809em;position:relative;line-height:50px;text-align:center;font-size:100%;*zoom:1;}".
            "nav a{color:inherit;text-decoration:none;display:block;}".
            "nav.side>div{position:relative;overflow-x:hidden;overflow-y:scroll;width:320px;height:100%;padding-bottom:64px;}".
            "div.nav p{height:32px;line-height:32px;padding:0 1.618em;margin:12px 0px 0px 0px;font-weight:700;text-transform:uppercase;font-size:85%;white-space:nowrap;-webkit-font-smoothing:antialiased}".
            "div.nav li>.current,div.nav li>ul{display:none;}".
            "div.nav li>a,div.nav li>label{display:block;}".
            "div.nav a,div.nav ul>li>label,div.nav ul>li>.current{width:300px;line-height:18px;padding:0.4045em 1.618em;}".
            "div.nav a,div.nav ul>li>label{cursor:pointer;}".
            "div.nav .current{font-weight:700;border-top:1px solid;border-bottom:1px solid #c9c9c9;}".
            "div.nav ul>li>ul>li>a{border-right:solid 1px #c9c9c9;font-size:90%;}".
            "div.nav ul>li>ul>li.h2>a{padding:0.4045em 2.427em;}".
            "div.nav ul>li>ul>li.h3>a{padding:.4045em 1.618em .4045em 4.045em;}".
            "div.nav ul>li>ul>li.h4>a{padding:.4045em 1.618em .4045em 5.663em;}".
            "div.nav ul>li>ul>li.h5>a{padding:.4045em 1.618em .4045em 7.281em;}".
            "div.nav ul>li>ul>li.h6>a{padding:.4045em 1.618em .4045em 8.899em;}".
            "div.nav ul,div.nav li,.breadcrumbs{margin:0px!important;padding:0px;list-style:none;}".
            "ul.breadcrumbs,.breadcrumbs li{display:inline-block;}".
            ".menu{display:inline-block;position:absolute;top:12px;right:20px;cursor:pointer;width:1.5em;height:1.5em;vertical-align:middle;padding:16px 24px 16px 24px;border:solid 1px rgba(255, 255, 255, 0.5);border-radius:5px;background:no-repeat center center url(\"data:image/svg+xml;charset=utf8,%3Csvg viewBox='0 0 30 30' xmlns='http://www.w3.org/2000/svg'%3E%3Cpath stroke='rgba(255, 255, 255, 0.5)' stroke-width='2' stroke-linecap='round' stroke-miterlimit='10' d='M4 7h22M4 15h22M4 23h22'/%3E%3C/svg%3E\");}".
            ".home{display:inline-block;max-width:16px;max-height:16px;line-height:16px;margin:0 5px 0 0;cursor:pointer;}".
            ".home::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAS1BMVEUAAAD8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/PyxWsjVAAAAGHRSTlMAx6oLLRnYiXFEvPbey518NQbsj11QKhQ+J0ktAAAAbElEQVQY053ISQ7DMAwEwRYpavHurPz/SwMoMpD46L4MpvBTB0zTP+R9z55Ebh0yQL5DaDA+0WVR3h3Gikp9iPKF9MIkQhSDwUlGHCLQxhLGJkpLZcNYS/tdyorP/DQ7HgBqKRUgHBDcw2X4AFsJCSXB/5UVAAAAAElFTkSuQmCC);}".
            "h1>a,h2>a,h3>a,h4>a,h5>a,h6>a{display:none;max-width:16px;max-height:24px;margin:-8px 0 0 5px;vertical-align:middle;}".
            "h1:hover>a,h2:hover>a,h3:hover>a,h4:hover>a,h5:hover>a,h6:hover>a{display:inline-block;text-decoration:none!important;}".
            "h1>a::before,h2>a::before,h3>a::before,h4>a::before,h5>a::before,h6>a::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAiBAMAAACkb0T0AAAAMFBMVEUAAAD8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/Pz8/PzS+N+gAAAAD3RSTlMABab5cVY73o4k2cJTPb1Q83MyAAAAh0lEQVQY02P4DwWkM7YxsNiDGJ8EGBgmgxiKDDkCLAwgAQn7hQxARiGj8v8/IIYDi2a9GZChHiAnMLmBjeGTaIBUAMcFaYaNvA2sC1kD9RkSOCwZF7IW/WdgSP4+yYHlP5Bx/r+RgDiIIfW5kVEfyHBgPMYgBLLdkoEBKABkfHZgyIa7h04MAOVty/RC/PhoAAAAAElFTkSuQmCC);}".
            "h1>a:hover::after,h2>a:hover::after,h3>a:hover::after,h4>a:hover::after,h5>a:hover::after,h6>a:hover::after{".
            "content:\"".self::$lang["link"]."\";display:block;padding:12px;position:absolute;margin:-8px 8px;font-weight:400;font-size:14px;background:rgba(0,0,0,.8);color:#fff;border-radius:4px;}".
            "input[type=radio]{display:none;}".
            "input[type=radio]:checked ~ ul{display:block;}".
            ".fig{margin-top:-12px;padding-bottom:12px;display:block;text-align:center;font-style:italic;}".
            "div.page{width:100%;padding:1.618em 3.236em;margin:auto;line-height:24px;}".
            "div.page ol{margin:0 0 24px 12px;padding-left:0px;}div.page ul{margin:0 0 24px 24px;list-style:disc outside;padding-left:0px;}".
            "div.page ol{list-style-type:none;counter-reset:list;}div.page ol li:before{counter-increment:list;content:counters(list,\".\") \". \";}".
            "div.pre{overflow-x:auto;margin:1px 0px 24px;}div.table{overflow-x:auto;margin:0px 0px 24px;}".
            "div.info,div.hint,div.warn{padding:12px;line-height:24px;margin-bottom:24px;}".
            "div.info>p,div.hint>p,div.warn>p{margin:0px;}".
            "div.info>p:first-child,div.hint>p:first-child,div.warn>p:first-child{display:block;font-weight:700;padding:2px 8px 2px;margin:-12px -12px 8px -12px;vertical-align:middle;}".
            "div.info>p:first-child>span,div.hint>p:first-child>span,div.warn>p:first-child>span{display:block;max-height:20px;margin:0px;vertical-align:middle;}".
            "div.info>p:first-child>span::before,div.hint>p:first-child>span::before,div.warn>p:first-child>span::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAApUlEQVQ4y82TwQnDMAxFpVJyLPiSbbJETr13Fo+RZZxLRghkgnSEEni9ONS0sg1tDv3wwfjbX7Iki/wVAAd4IPBCiHuudrkHVvJYgb50eaOO7cMkpv0eeQGukYuRiUsNvBFpSvTJ0P2un0Sk+6Le3WEG58yBFrjt61r7Qqbij0gLIX3CaPgOqtqoaiMig6GPtTbOwCVyLraxMEj3yPIgHTLKv3ymJySzt16bW/sWAAAAAElFTkSuQmCC);}".
            "p>div:last-child,dd>*:last-child,td>*:last-child,li>ol,li>ul{margin-bottom:0px!important;}".
            "img{border:0px;}img.imgt{display:inline-block;max-height:22px!important;padding:0px;margin:-4px 0px 0px 0px;vertical-align:middle;}img.imgl{float:left;margin:0px 12px 12px 0px;}img.imgr{float:right;margin:0px 0px 12px 12px;}div.imgc{text-align:center;padding:0px;margin:0 12px 0 0;clear:both;}img.imgc{max-width:100%%;}img.imgw{width:100%;margin-bottom:12px;clear:both;}".
            ".btn{border-radius:2px;line-height:normal;white-space:nowrap;color:inherit;text-align:center;cursor:pointer;font-size:100%;padding:4px 12px 8px;border:1px solid rgba(0,0,0,.1);text-decoration:none;box-shadow:inset 0 1px 2px -1px hsla(0,0%,100%,.5),inset 0 -2px 0 0 rgba(0,0,0,.1);vertical-align:middle;*zoom:1;user-select:none;transition:all .1s linear}".
            ".prev{float:left;}.prev::before{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABIAAAASCAMAAABhEH5lAAAANlBMVEUAAABAQEBAQEBAQEBAQEBAQEBAQEBBQUFAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAWW5SEAAAAEnRSTlMA/fC9r2kXAjMN34F3ZlUu6B40Y5wGAAAAVElEQVQY08XPSw6AIAwEUIbS8lFE739Zq6luGtbM8iXTTMOCZGECiCX/MhIQTyCNz+SRrUc1MWKVvR5KYCN6pUFDRtqq4Sql9IYJ+aI/70f4qdOHblOhAuUcC5KnAAAAAElFTkSuQmCC);}".
            ".next{float:right;}.next::after{content:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABIAAAASBAMAAACk4JNkAAAAJFBMVEUAAABAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEC4lvDfAAAAC3RSTlMAx711ZjlFPh3zLASjkrYAAABDSURBVAjXY6AUsGhvcgAzOKR3797YAGIx7t5mvVsAxPLevZ159xYQS3v3zgLrTSDW7tTQBcy7ESyELEIHwhSEyQjbAAH1HsMY8tCHAAAAAElFTkSuQmCC);}".
            "@media screen and (max-width:991.98px){nav.mobile{display:block;}nav.side{display:none;}#menuchk:checked ~ nav.side{display:block;}#_m{margin-left:0px;}}");
            foreach(self::$toc as $k=>$v) if($v[0] == "1") fwrite($f,"#_".$k.":checked ~ nav div ul li[rel=".$k."]>.toc,");
            fwrite($f, "div.page{display:none;}");
            foreach(self::$toc as $k=>$v) if($v[0] == "1") fwrite($f,"#_".$k.":checked ~ nav div ul li[rel=".$k."]>ul,#_".$k.":checked ~ nav div ul li[rel=".$k."]>.current,#_".$k.":checked ~ div div[rel=".$k."],");
            fwrite($f, "#_:checked ~ div div[rel=_]{display:block;}</style>\n  <style rel=\"theme\">".trim(strtr($theme, [ "\r" => "", "\n" => "" ]))."</style>\n</head>\n<body>\n  <div class=\"frame content\">\n    ");
            if(self::$H)fwrite($f, "<input type=\"radio\" name=\"page\" id=\"_\" checked>");
            foreach(self::$toc as $k=>$v)
                if($v[0] == "1") {
                    fwrite($f, "<input type=\"radio\" name=\"page\" id=\"_".$k."\"".(!self::$H ? " checked":"").">");
                    self::$H = 1;
                }
            fwrite($f, (self::$H ? "\n" : ""). "    <input type=\"checkbox\" id=\"menuchk\" style=\"display:none;\"><nav class=\"side nav\"><div>\n      <div class=\"title\"><a href=\"".self::$lang["url"]."\">".(!empty($titimg) ? $titimg.self::$lang["title"] : $title)."</a><div class=\"version\">".self::$lang["version"]."</div>".
                "<input id=\"_q\" class=\"search\" type=\"text\" required=\"required\" onkeyup=\"s(this.value);\"></div>".
                "      <div id=\"_s\" class=\"nav\"></div>\n      <div id=\"_t\" class=\"nav\">\n");
            $H = 0;
            foreach(self::$toc as $k=>$v) {
                if($v[0] == "0") {
                    if($H > 1) fwrite($f, "        </ul></li>\n");
                    if($H) fwrite($f, "        </ul>\n");
                    fwrite($f, "        <p>".$v[1]."</p>\n");
                } else
                if($v[0] == "1") {
                    if(!$H) fwrite($f, "        <ul>\n");
                    else fwrite($f, "        </ul></li>\n");
                    fwrite($f, "        <li rel=\"".$k."\"><label class=\"toc\" for=\"_".$k."\">".$v[1]."</label><div class=\"current\">".$v[1]."</div><ul>\n");
                } else
                    fwrite($f, "          <li class=\"h".$v[0]."\"><a href=\"#".$k."\" onclick=\"m()\">".$v[1]."</a></li>\n");
                $H = intval($v[0]);
            }
            if($H > 1) fwrite($f, "        </ul></li>\n");
            if($H) fwrite($f, "        </ul>\n");
            fwrite($f, "      </div>\n".
                "    </div></nav>\n".
                "    <div id=\"_m\">\n".
                "      <nav class=\"mobile title\">".$title."<label for=\"menuchk\" class=\"menu\"></label></nav>\n".self::$out.
                "\n      <footer><hr><p>© Copyright ".self::$lang["copy"]."<br><small>Generated by <a href=\"https://gitlab.com/bztsrc/gendoc\">gendoc</a> v".self::$version."</small></p></footer>\n".
                "    </div>\n".
                "  </div>\n".
                /* embedded JavaScript, optional, minimal, vanilla (jQuery-less) code. Licensed under CC-BY */
                "<script>".
                "function m(){document.getElementById(\"menuchk\").checked=false;}".
                /* onclick handler that changes the url *and* switches pages too */
                "function c(s){".
                  "var r=document.getElementById(s);".
                  "if(r!=undefined){".
                    "if(r.tagName==\"INPUT\")r.checked=true;".
                    "else document.getElementById(\"_\"+r.parentNode.getAttribute(\"rel\")).checked=true;".
                  "}m();}".
                /* search function */
                "function s(s){".
                  "var r=document.getElementById(\"_s\"),p=document.getElementById(\"_m\").getElementsByClassName(\"page\"),n,i,j,a,b,c,d;".
                  "if(s){".
                    "s=s.toLowerCase();document.getElementById(\"_t\").style.display=\"none\";r.style.display=\"block\";".
                    "while(r.firstChild)r.removeChild(r.firstChild);n=document.createElement(\"p\");n.appendChild(document.createTextNode(\"".self::$lang["rslt"]."\"));r.appendChild(n);".
                    "for(i=1;i<p.length;i++){".
                      "a=p[i].getAttribute(\"rel\");b=\"\";c=p[i].childNodes;d=p[i].getElementsByTagName(\"H1\")[0].innerText;".
                      "for(j=1;j<c.length && c[j].className!=\"btn prev\";j++){".
                        "if(c[j].id!=undefined&&c[j].id!=\"\"){".
                          "a=c[j].id;d=c[j].innerText;".
                        "}else if(a!=b&&c[j].innerText!=undefined&&c[j].innerText.toLowerCase().indexOf(s)!=-1){".
                          "b=a;n=document.createElement(\"a\");n.appendChild(document.createTextNode(d));n.setAttribute(\"href\",\"#\"+a);n.setAttribute(\"onclick\",\"c('\"+a+\"');\");r.appendChild(n);".
                        "}}}".
                  "}else{".
                    "document.getElementById(\"_t\").style.display=\"block\";r.style.display=\"none\";}}".
                "document.addEventListener(\"DOMContentLoaded\",function(e){var i,r,n;document.getElementById(\"_q\").style.display=\"inline-block\";".
                /* the query URL hack */
                "if(document.location.href.indexOf(\"?\")!=-1)document.location.href=document.location.href.replace(\"?\",\"#\");else{".
                /* replace labels with "a" tags that change the url too */
                "r=document.querySelectorAll(\"LABEL:not(.menu)\");".
                "while(r.length){".
                "l=r[0].getAttribute(\"for\").substr(1);".
                "n=document.createElement(\"a\");n.appendChild(document.createTextNode(r[0].innerText));".
                "n.setAttribute(\"href\",\"#\"+l);n.setAttribute(\"onclick\",\"c('\"+(l!=\"\"?l:\"_\")+\"');\");".
                "if(r[0].getAttribute(\"class\")!=undefined)n.setAttribute(\"class\",r[0].getAttribute(\"class\"));".
                "if(r[0].getAttribute(\"title\")!=undefined&&l!=\"\")n.setAttribute(\"title\",r[0].getAttribute(\"title\"));".
                "if(r[0].getAttribute(\"accesskey\")!=undefined)n.setAttribute(\"accesskey\",r[0].getAttribute(\"accesskey\"));".
                "r[0].parentNode.replaceChild(n,r[0]);".
                "r=document.querySelectorAll(\"LABEL:not(.menu)\");".
                /* switch to a page which is detected from the url */
                "}try{c(document.location.href.split(\"#\")[1]);}catch(e){}}});".
                "</script>\n</body>\n</html>\n");
            fclose($f);
        }
    }
}

/*** collect info ***/
$files = $_SERVER['argv'];
array_shift($files);
$outfn = $files[0];
$l = strrpos($outfn, '.');
if($l !== false) {
    $plugin = dirname(__FILE__)."/plugins/fmt_".substr($outfn, $l + 1).".php";
    if(file_exists($plugin)) {
        gendoc::$writer = require_once $plugin;
        if(empty(gendoc::$writer) || !method_exists(gendoc::$writer, "output")) {
            gendoc::report_error("the gendoc_".substr($outfn, $l + 1)." plugin does not support writing files.");
            die;
        }
    }
}
array_shift($files);
/* load all highlight rules */
$plugins = @glob(dirname(__FILE__)."/plugins/hl_*.json");
foreach($plugins as $fn) {
    $v = basename($fn);
    $type = substr($v, 3, strlen($v) - 8);
    $v = @json_decode(str_replace("\\\"", "\"", str_replace("\\", "\\\\",preg_replace("/\/\*.*?\*\//ims","",@file_get_contents($fn)))),false);
    if(is_array($v) && !empty($v)) gendoc::$rules[$type] = $v;
}
/* iterate on input files */
foreach($files as $fn)
    gendoc::include($fn);
gendoc::output($outfn);
if(gendoc::$err)
    echo("gendoc: there were errors during generation, your documentation is although saved, but probably is incomplete!\r\n");
