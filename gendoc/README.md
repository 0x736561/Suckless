Generate Documentation
======================

The **gendoc** tool is a small tool that generates documentation into a formatted static HTML. It is similar to Sphinx
and ReadtheDocs, but much better and way cooler, because it has no server component parts and works without JavaScript too.

[Online Demo](https://bztsrc.gitlab.io/gendoc)

I've tried to use the tools like everybody else. I've really tried and wanted to use ReadtheDocs so that I don't have to create
my own. But the deeper I got into it, the more worried I become, not just because of its overcomplicated structure and
dependencies. When I've realized that it actually violates GDPR and you are forced to enable JavaScript just to allow Google to
track you (without notice nor opt-out), and you can't just save an RtD doc on your computer, you're depending on the mercy of
RtD and its on-line availability (otherwise you're cut off from *your own* documentation entirely), then I've gave up on RtD and
rather implemented a [suckless](https://suckless.org) solution.

Usage
-----

### ANSI C Version

The **gendoc** generator is implemented in ANSI C. Completely and entirely dependency free, single source file about 100k
(includes syntax highlighter, regexp matcher, JSON parser, API documentation generator etc.). Super-simple, super-fast.

To compile run

```sh
$ gcc gendoc.c -o gendoc
```

Yeah, that simple. As I've said, no dependencies. To generate documentation into a W3C valid, self-contained HTML5 file use

```sh
$ ./gendoc <output.html> <input file> [input file] [input file...]
```

NOTE: the ANSI C version only supports the syntax highlight plugins, but not the others (like the file format readers / writers),
although besides of gendoc tags, it has a built-in parser for MarkDown too, based on [smu](https://github.com/Gottox/smu).

### PHP Version

And there's the PHP script version, which supports plugins. With those it can read various input formats and can generate into
any arbitrary documentation format.

```sh
$ php ./gendoc.php <output> <input file> [input file] [input file...]
```

Yeah, that simple. No dependencies, everything is included in this single file 90 kilobytes of code (just like with the C version).
The output's format is detected by the file extension, and in lack of a writer plugin for that extension, defaults to the built-in
HTML writer. Input files likewise, format detected by file extension, and in lack of a reader plugin defaults to gendoc tags. The
PHP version of the MarkDown parser is based on [ParseDown](https://parsedown.org).

Documentation
-------------

Stylishly, **gendoc**'s own manual is available in a **gendoc** document, [here](https://bztsrc.gitlab.io/gendoc). You can press
right-click on it, select "Save As" and take that documentation with you. The dependency-free static HTML file is competely
self-contained, with embedded style sheets, images, etc. You can keep it and you don't have to worry about gitlab going off-line.

If you need a more complex, more real-life example with multiple files and MarkDown too, see
[this manual.xml](https://codeberg.org/tirnanog/editor/src/branch/main/docs/en). It takes good use of all the **gendoc** features,
and its generated HTML output looks like [this](https://tirnanog.codeberg.page/manual_en.html).

Known Issues
------------

The built-in validator admitedly isn't the best, but it's fast and detects all the typical common mistakes (some unmatched
tags in deeply nested structures might slip through and only reported when the next heading reached).

License
-------

The generator tool is available under [GPLv3+](https://gitlab.com/bztsrc/gendoc/blob/main/LICENSE) or any later version of
that license. That few icons embedded into the output originate from public FontAwesome icons and they are licensed
[CC-BY-4.0](https://github.com/FontAwesome/Font-Awesome/blob/master/LICENSE.txt). I would like to say that the embedded CSS is
from RtD, but the thruth is, RtD's style was such a mess I had to rewrite everything from scratch. So it looks like and feels
like the original RtD stylesheet, but it is a complete rewrite from ground up, licensed under **CC-BY**. That minimal
vanilla JavaScript code that gets into the documents (to provide search results) is also licensed under **CC-BY**.

Authors
-------

bzt
