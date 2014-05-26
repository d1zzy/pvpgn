--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- Config table can be extended here with your own variables
-- values are preloaded from bnetd.conf
config = {
	-- Quiz settings
	quiz = true,
	quiz_filelist = "misc, dota, warcraft", -- display available files in "/quiz start"
	quiz_competitive_mode = true, -- top players loses half of points which last player received; at the end top of records loses half of points which players received in current game
	quiz_max_questions = 100, -- from start to end
	quiz_question_delay = 5, -- delay before send next question
	quiz_hint_delay = 20, -- delay between prompts
	quiz_users_in_top = 15, -- how many users display in TOP list
	quiz_channel = nil, -- (do not modify!) channel when quiz has started (it assigned with start)
	
}

