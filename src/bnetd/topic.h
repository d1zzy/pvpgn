/*
*	Copyright (C) 2015  xboi209
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INCLUDED_TOPIC_TYPES
#define INCLUDED_TOPIC_TYPES

#include <memory>
#include <string>
#include <vector>

#include "connection.h"
#include "common/list.h"
#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_TOPIC_PROTOS
#define INCLUDED_TOPIC_PROTOS
namespace pvpgn
{

	namespace bnetd
	{

		class class_topic
		{
		//all public member functions of class_topic verify channel name and topic length
		public:
			class_topic();

			std::string get(const std::string channel_name);
			bool set(const std::string channel_name, const std::string topic_text, bool do_save);
			bool display(t_connection * c, const std::string channel_name);
		private:
			struct t_topic
			{
				std::string channel_name;
				std::string topicstr;
				bool save;
			};

			class class_topiclist
			{
			public:
				static bool IsHeadLoaded;
				std::shared_ptr<class_topic::t_topic> get(const std::string channel_name);
				bool save();
				void add(std::string channel_name, std::string topic_text, bool do_save);
			private:
				static std::vector<std::shared_ptr<class_topic::t_topic>> Head;
			} topiclist;
		};

	} //namespace bnetd

} //namespace pvpgn

#endif
#endif