<?php
/*
 * plugins/api_python.php
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
 * @brief Look for doctype API specification strings in Python sources
 *
 */

function gendoc_api_python($str)
{
    /* match comments before methods that look like:
     *
     * ##
     * # descripton
     * # more description
     * # @param parameter definition
     * # @return return value definition
     * def somemethod():
     */
    if(preg_match_all("/##[^#](.*?)def(.*?)$/ims", $str, $M, PREG_SET_ORDER)) {
        foreach($M as $m) {
            /* format the matches */
            gendoc::data_list_open();
            gendoc::data_topic_open();
            gendoc::source_code(trim($m[2]), "python");
            gendoc::data_topic_close();
            gendoc::data_description_open();
            $t = 0;
            foreach(explode("\n", $m[1]) as $dsc) {
                $dsc = htmlspecialchars(trim(preg_replace("/^[\ \t]*#[\ \t]*/", "", $dsc)));
                if(!empty($dsc)) {
                    if(substr($dsc, 0, 7) == "@param ") {
                        if(!$t) {
                            $t = 1;
                            gendoc::table_open();
                            gendoc::table_row_open();
                            gendoc::table_header_open();
                            gendoc::text(gendoc::$lang["args"]);
                            gendoc::table_header_close();
                            gendoc::table_row_close();
                        }
                        gendoc::table_row_open();
                        gendoc::table_cell_open();
                        gendoc::text(trim(substr($dsc, 7)));
                        gendoc::table_cell_close();
                        gendoc::table_row_close();
                    } else
                    if(substr($dsc, 0, 8) == "@return ") {
                        if(!$t) {
                            $t = 1;
                            gendoc::table_open();
                        }
                        gendoc::table_row_open();
                        gendoc::table_header_open();
                        gendoc::text(gendoc::$lang["rval"]);
                        gendoc::table_header_close();
                        gendoc::table_row_close();
                        gendoc::table_row_open();
                        gendoc::table_cell_open();
                        gendoc::text(trim(substr($dsc, 8)));
                        gendoc::table_cell_close();
                        gendoc::table_row_close();
                    } else
                        gendoc::text($dsc." ");
                }
            }
            if($t)
                gendoc::table_close();
            gendoc::data_description_close();
            gendoc::data_list_close();
            gendoc::line_break();
        }
    }
}
