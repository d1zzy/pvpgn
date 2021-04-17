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

#include "common/setup_before.h"
#include "topic.h"

#include <fstream>
#include <memory>
#include <new>
#include <regex>
#include <string>
#include <utility>

#include "compat/strcasecmp.h"

#include "common/eventlog.h"
#include "common/field_sizes.h"

#include "message.h"
#include "prefs.h"

#include "common/setup_after.h"

namespace pvpgn
{

	namespace bnetd
	{
		std::vector<std::shared_ptr<class_topic::t_topic>> class_topic::class_topiclist::Head;

		bool class_topic::class_topiclist::IsHeadLoaded = false;

		class_topic::class_topic()
		{
			//Already loaded, do not load again
			if (this->topiclist.IsHeadLoaded == true)
				return;

			std::ifstream topicfile_stream(prefs_get_topicfile());
			if (!topicfile_stream)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "couldn't open topic file");
				return;
			}

			std::string strLine;
			std::smatch match;
			const std::regex rgx("(.*)\t(.*)"); // tab character separates the channel name and topic

			//loop through each line in topic file
			while (std::getline(topicfile_stream, strLine))
			{
				//skip empty lines
				if (strLine.empty() == true)
					continue;

				if (!std::regex_search(strLine, match, rgx))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "Invalid line in topic file ({})", strLine.c_str());
					continue;
				}

				//check if current line in file already exists in Head
				if (this->topiclist.get(match[1].str()) != nullptr)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "Duplicate line for channel {} in topic file", match[1].str().c_str());
					continue;
				}

				//save current line to Head
				this->topiclist.add(match[1].str(), match[2].str(), true);
			}

			this->topiclist.IsHeadLoaded = true;
		}

		//Get channel_name's topic string
		std::string class_topic::get(const std::string channel_name)
		{
			if ((channel_name.size() + 1) > MAX_CHANNELNAME_LEN || channel_name.empty() == true)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid channel name length");
				return std::string();
			}

			auto topic = this->topiclist.get(channel_name);
			if (topic == nullptr)
				return std::string();

			return topic->topicstr;
		}

		//Sets channel_name's topic
		bool class_topic::set(const std::string channel_name, const std::string topic_text, bool do_save)
		{
			if ((channel_name.size() + 1) > MAX_CHANNELNAME_LEN || channel_name.empty() == true)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid channel name length");
				return false;
			}

			if ((topic_text.size() + 1) > MAX_TOPIC_LEN || topic_text.empty() == true)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid topic length");
				return false;
			}

			auto topic = this->topiclist.get(channel_name);

			if (topic != nullptr)
			{
				eventlog(eventlog_level_trace, __FUNCTION__, "Setting <{}>'s topic to <{}>", channel_name.c_str(), topic_text.c_str());
				topic->topicstr = topic_text;
			}
			else
			{
				eventlog(eventlog_level_trace, __FUNCTION__, "Adding <{}:{}> to topiclist", channel_name.c_str(), topic_text.c_str());
				this->topiclist.add(channel_name, topic_text, do_save);
			}

			if (do_save == true)
			{
				if (this->topiclist.save() == false)
				{
					eventlog(eventlog_level_error, __FUNCTION__, "error saving topic list");
					return false;
				}
			}

			return true;
		}

		// Displays channel_name's topic to connection c
		bool class_topic::display(t_connection * c, const std::string channel_name)
		{
			if ((channel_name.size() + 1) > MAX_CHANNELNAME_LEN || channel_name.empty() == true)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "got invalid channel name length");
				return false;
			}

			auto topic = this->topiclist.get(channel_name);
			if (topic == nullptr)
				return false;

			auto topicstr = topic->topicstr;

			if (topicstr.empty())
			{
				eventlog(eventlog_level_error, __FUNCTION__, "topic is empty");
				return false;
			}

			//send parts of topic string as separate message if there's a newline character
			std::regex rgx("\\\\n+");
			std::sregex_token_iterator iter(topicstr.begin(), topicstr.end(), rgx, -1), end;
			for (bool first = true; iter != end; ++iter)
			{
				std::string msg(iter->str());
				if (first == true)
				{
					msg.insert(0, topic->channel_name + " topic: ");
					first = false;
				}
				message_send_text(c, message_type_info, c, msg);
			}

			return true;
		}


		//Get t_topic pointer of channel_name
		std::shared_ptr<class_topic::t_topic> class_topic::class_topiclist::get(const std::string channel_name)
		{
			for (auto topic : this->Head)
			{
				if (strcasecmp(channel_name.c_str(), topic->channel_name.c_str()) == 0)
					return topic;
			}
			eventlog(eventlog_level_debug, __FUNCTION__, "returning nullptr");

			return nullptr;
		}

		//Saves data from Head vector to topic file
		bool class_topic::class_topiclist::save()
		{
			std::fstream topicfile_stream(prefs_get_topicfile(), std::ofstream::app);
			if (!topicfile_stream)
			{
				eventlog(eventlog_level_error, __FUNCTION__, "couldn't open topic file");
				return false;
			}

			std::string strLine;
			std::smatch match;
			const std::regex rgx("(.*)\t(.*)"); // tab character separates the channel name and topic

			//Check if data in Head vector already exists in topic file
			while (std::getline(topicfile_stream, strLine))
			{
				if (!std::regex_search(strLine, match, rgx))
				{
					eventlog(eventlog_level_error, __FUNCTION__, "Invalid line in topic file ({})", strLine.c_str());
					continue;
				}

				for (auto topic : this->Head)
				{
					if (topic->save == true)
					{
						if (match[1].str() == topic->channel_name)
						{
							break;
						}
						else
						{
							topicfile_stream << topic->channel_name << "\t" << topic->topicstr << std::endl;
							break;
						}
					}
				}
			}
			return true;
		}

		//Adds a new pointer to the Head vector
		void class_topic::class_topiclist::add(std::string channel_name, std::string topic_text, bool do_save)
		{
			auto topic = std::make_shared<class_topic::t_topic>(class_topic::t_topic{ channel_name, topic_text, do_save });
			this->Head.push_back(std::move(topic));
		}

	}

}