/*
 * Copyright (C) 2004  CreepLord (creeplord@pvpgn.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int u2d(char * file)
{
    FILE * orig_file;
    FILE * new_file;
    char temp_file[256];
    int ch;
    int prev_ch = 0;
    
    sprintf(temp_file, "%s.tmp", file);
    orig_file = fopen(file, "rb");
    new_file = fopen(temp_file, "wb");
    
    while ((ch = getc(orig_file)) != EOF)
    {
        if ((ch == 0x0A) && (prev_ch != 0x0D))
            putc(0x0D, new_file);
        
        putc(ch, new_file);
        prev_ch = ch;
    }
    
    fclose(orig_file);
    fclose(new_file);
    remove(file);
    rename(temp_file,file);
    remove(temp_file);
    return 0;
}

int main (int argc, char **argv)
{
    int i;
    
    for (i = 1; i < argc; i++)
        u2d(argv[i]);
    
    return 0;
}

