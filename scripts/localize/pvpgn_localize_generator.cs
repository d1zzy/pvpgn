/*
Copyright (c) 2014 HarpyWar (harpywar@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Xml.Serialization;

namespace pvpgn_localize_generator
{
    class Program
    {
        /// <summary>
        /// C++ localization function name
        /// </summary>
        private const string func_name = "localize";
        private const string outfile = "output.xml";

        static Root _data = new Root();

        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
                Console.WriteLine("This utility generates XML file for next translation from hardcoded text arguments in function {0}(...) that in *.cpp files\n(c) 2014 HarpyWar (harpywar@gmail.com)", func_name);
                Console.WriteLine("\nUsage: {0} [path to 'src/bnetd']\n", AppDomain.CurrentDomain.FriendlyName);

                Environment.Exit(0);
            }
            var dirpath = args[0];

            // process all files in directory
            foreach(var f in Directory.GetFiles(dirpath))
            {
                parse_file(f);
            }

            // serialize data to xml
            var ser = new XmlSerializer(typeof(Root));
            using (var fs = new FileStream(outfile, FileMode.Create))
            {
                ser.Serialize(fs, _data);
            }

            Console.WriteLine("\n{0} items saved in {1}: ", _data.Items.Count, outfile);
            Console.WriteLine("\nPress any key to exit...");
            Console.ReadKey();
        }

        /// <summary>
        /// Parse a single file
        /// </summary>
        /// <param name="filepath"></param>
        private static void parse_file(string filepath)
        {
            string[] lines = File.ReadAllLines(filepath);
            string filename = Path.GetFileName(filepath);

            string text, f, function = string.Empty;
            int i = 0;
            foreach (string s in lines)
            {
                i++;
                try
                {
                    if ((f = is_function(s)) != null)
                        function = f; // remember last function

                    if ((text = find_localize_text(s)) == null)
                        continue;

                    _data.Items.Add(new Root.StringItem()
                    {
                        File = filename,
                        Function = function,
                        Original = text,
                        Translate = " "
                    });

                    Console.WriteLine("{0}, {1}(): {2}", filename, function, text);
                }
                catch(Exception e)
                {
                    Console.WriteLine("Error on parse file \"{0}\" on line #{1}: {2}", filename, i, s);
                    Console.WriteLine(e.Message);
                }
            }
        }

        /// <summary>
        /// Return a text from the first string argument of the func_name
        /// </summary>
        /// <param name="line"></param>
        /// <returns>text or null</returns>
        private static string find_localize_text(string line)
        {
            int func_pos, bracket1_pos, quote1_pos, quote2_pos;
            func_pos = bracket1_pos = quote1_pos = quote2_pos = -1;

            string text = null;

            for (int i = 0; i < line.Length; i++)
            {
                if (func_pos >= 0)
                {
                    if (bracket1_pos > 0)
                    {
                        if (quote1_pos > 0)
                        {
                            if (quote2_pos > 0)
                            {
                                text = line.Substring(quote1_pos, quote2_pos - quote1_pos);
                                break;
                            }
                            // 3) find last quote
                            if (line.Substring(i, 1) == "\"" && line.Substring(i-1, 1) != "\\")
                                quote2_pos = i;
                            continue;
                        }
                        // 3) find first quote
                        if (line.Substring(i, 1) == "\"")
                            quote1_pos = ++i;
                        continue;
                    }
                    // 2) find first bracket
                    if (line.Substring(i, 1) == "(")
                        bracket1_pos = i;
                    continue;
                }
                // 1) find function name
                if (line.Substring(i, (i+func_name.Length > line.Length) ? line.Length-i : func_name.Length) == func_name)
                    func_pos = i;
            }
            return escape_text(text);
        }

        /// <summary>
        /// Filter text corresponding XML rules
        /// </summary>
        /// <param name="text"></param>
        /// <returns>text or null (if null passed)</returns>
        private static string escape_text(string text)
        {
            if (text == null)
                return null;

            text = text.Replace("\\\"", "\"");
            text = text.Replace("<", "&lt;");
            text = text.Replace(">", "&gt;");
            return text;
        }


        /// <summary>
        /// Is word a function?
        /// </summary>
        /// <param name="line"></param>
        /// <returns>string or null</returns>
        private static string is_function(string line)
        {
            int j;
            line = line.Trim();
            if (line.Length == 0)
                return null;

            // last line must have ) or {
            if (line[line.Length - 1] == ')' || line[line.Length - 1] == '{')
            {
                string[] words = line.Split();
                if (words.Length > 0)
                {
                    bool bad = false;
                    // exclude small words and reserved words that can not be at the beginning of the function definition
                    foreach (string r in reserved_words)
                        if (r.Length > words[0].Length)
                            continue;
                        else if (words[0].Substring(0, r.Length) == r)
                            bad = true;

                    if (!bad)
                    {
                        // find function name in words
                        for (int i = 0; i < words.Length; i++)
                        {
                            if (words[i].Trim() == string.Empty)
                                continue;

                            if (words[i][0] == '(')
                                return words[i - 1];
                            else if ((j = words[i].IndexOf("(", 0)) != -1)
                                return words[i].Substring(0, j);
                        }

                    }
                }
            }
            return null;
        }

        static string[] reserved_words = new string[] { 

            "while",
            "switch",
            "class",
            "new",
            "goto",
            "for",
            "sizeof",
            "struct",
            "throw",
            "try",
            "catch",
            "typedef",
            "enum",
            "if", "else",
            "{","}",
            "(",")",
            "<<",">>",
            "||","|",
            "&&","&",
            "//", "/", "*", // comments
            "!",
        };
    }

#region Serializer Class

    [XmlRoot("root")]
    public class Root
    {
        public Root()
        {
            Items = new List<StringItem>();
        }

        [XmlElement("meta")]
        public Meta meta = new Meta();

        [XmlArray("items"), XmlArrayItem(typeof(StringItem), ElementName = "item")]
        public List<StringItem> Items { get; set; }

        public class Meta
        {
            public Meta()
            {
                language = new LanguageItem();
                Authors = new List<AuthorItem>() {
                    new AuthorItem()
                };
            }

            [XmlElement("language"), DefaultValue("change_me")]
            public LanguageItem language { get; set; }

            [XmlArray("authors"), XmlArrayItem(typeof(AuthorItem), ElementName = "author")]
            public List<AuthorItem> Authors { get; set; }

            public class LanguageItem
            {
                [XmlAttribute("tag")]
                public string Tag = "enUS";
                [XmlText]
                public string Default = "English";
            }
            public class AuthorItem
            {
                [XmlAttribute("name")]
                public string Name = "nomad";
                [XmlAttribute("email")]
                public string Email="nomad@example.com";
            }
        }


        public class StringItem
        {
            [XmlAttribute("file")]
            public string File;
            [XmlAttribute("function")]
            public string Function;
            [XmlElement("original")]
            public string Original;
            [XmlElement("translate")]
            public string Translate;
        }

#endregion

    }
}
