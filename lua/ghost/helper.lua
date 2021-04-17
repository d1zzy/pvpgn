--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- table key=value (user=bot)
-- it fills first when user uses a command
gh_user2bot = {}

-- array with users who will not receive a response from /ping
silentpings = {}

-- return a user mapped bot name
-- (nil if bot not mapped)
function gh_get_userbot_name(username)
	if not next(gh_user2bot) or not gh_user2bot[username] then return nil end
	return gh_user2bot[username].bot
end
-- return a user mapped game name that created by bot
function gh_get_userbot_game(username)
	if not next(gh_user2bot) or not gh_user2bot[username] then return nil end
	return gh_user2bot[username].game
end
-- return a user mapped game type that created by bot
function gh_get_userbot_gametype(username)
	if not next(gh_user2bot) or not gh_user2bot[username] then return nil end
	return gh_user2bot[username].gametype
end
-- find gamename in games and return owner of the game (username)
-- return false if not found
function gh_find_userbot_by_game(gamename)
	for u,bot in pairs(gh_user2bot) do
		if (bot.game == gamename) then return u end
	end
	return false
end

function gh_set_userbot(username, botname, gamename, gametype)
	gh_user2bot[username] = { bot = botname, game = gamename, type = gametype, time = os.time() }
end
function gh_del_userbot(username)
	gh_user2bot[username] = nil
end

-- save table to file
function gh_save_userbots(filename)
	table.save(gh_user2bot, filename)
	return true
end
-- load table from file
function gh_load_userbots(filename)
	gh_user2bot = table.load(filename)
	
	-- debug info
	if next(gh_user2bot) then
		TRACE("Users assigned to bots:")
		for u,bot in pairs(gh_user2bot) do
			TRACE(u..": "..bot.bot.." ("..bot.game..", "..bot.time..")")
		end
	end
end




-- add user into a silenttable
function gh_set_silentflag(username)
	table.insert(silentpings, account.name)
end
-- return true if user in a silenttable, and remove it
function gh_get_silentflag(username)
	for k,v in pairs(silentpings) do
		if v == username then
			table.remove(silentpings, k)
			return true
		end
	end
	return false
end


-- Get path to GHost directory
function gh_directory()
	return config.scriptdir .. "/ghost"
end



-- is user owner of bot in current game
function gh_is_owner(account)
	-- 1) user in game
	if not account.game_id then return false end

	local game = game_get_by_id(account.game_id)
	-- 2) user owner of the bot in current game
	if not game.owner == gh_get_userbot_name(account.name) then return false end
	
	return true
end




-- is bot in authorized config list?
function gh_is_bot(username)
	for i,bot in pairs(config.ghost_bots) do
		if string.lower(bot) == string.lower(username) then return true end
	end
	return false
end

	
-- redirect command from user to bot through PvPGN
function gh_message_send(botname, text)
	-- send message to ghost from the server 
	api.message_send_text(botname, message_type_whisper, nil, text)
end

-- return bot name (the best for user by ping)
-- return nil if no available bots
function gh_select_bot(username)
	local pings = account_get_botping(username)
	local botname = nil
	
	-- iterate all bots in config
	for i,bot in pairs(config.ghost_bots) do
		local is_found = false
		for k,v in pairs(pings) do
			if bot == v.bot then is_found = true end
		end
		-- if user has has no ping for the bot yet - use it
		if not is_found then
			botname = bot
		end
	end
	
	-- if user has all bots pings from config
	if not botname then
		-- select the best bot by ping (the table pings already sorted by ping)
		botname = pings[1].bot
	end

	local botacc = api.account_get_by_name(botname)
	-- if bot is offline then use first available
	-- FIXME: use next available by the best ping?
	if botacc.online == "false" then
		botname = nil
		for i,bot in pairs(config.ghost_bots) do
			botacc = api.account_get_by_name(bot)
			if botacc.online == "true" then return bot end
		end
	end
	
	return botname
end


