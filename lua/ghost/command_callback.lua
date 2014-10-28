--[[
	Connection interface between PvPGN and GHost
	https://github.com/OHSystem/ohsystem/issues/279

	
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--



---------------------------------
--- GHOST -> PVPGN (CALLBACK) ---
---------------------------------

-- /ghost [cmd] [code] {args}
function command_ghost(account, text)
	if not config.ghost then return 1 end
	
	-- restrict commands for authorized bots only
	if not gh_is_bot(account.name) then return 1 end
	
	local args = split_command(text, 5)
	local cmd = args[1]
	local code = args[2]
	
	if code ~= "ok" then
		if code == "err" then
			local err_message = args[3]
			-- HINT: you can handle error here
		end
		return -1
	end
	
	-- /ghost init
	if (cmd == "init") then
		INFO("GHost bot is connected (" .. account.name .. ")")
		
	-- /ghost gameresult [players] [ratings] [results]
	elseif (cmd == "gameresult") then
		gh_callback_gameresult(args[3], args[4], args[5])

	-- /ghost host [code] [user] [gamename]
	elseif (cmd == "host") then
		args = split_command(text, 4) -- support game name with spaces
		gh_callback_host(args[3], account.name, args[4])
	-- /ghost chost [code] [user] [gamename]
	elseif (cmd == "chost") then
		args = split_command(text, 4) -- support game name with spaces
		gh_callback_chost(args[3], account.name, args[4])
		
	-- /ghost unhost [code] [user] [gamename]
	elseif (cmd == "unhost") then
		gh_callback_unhost(args[3], args[4])
		
	-- /ghost ping [code] [user] [players] [pings]
	elseif (cmd == "ping") then
		gh_callback_ping(args[3], args[4], args[5])
		
	-- /ghost swap [code] [user] [username1] [username2]
	elseif (cmd == "swap") then
		-- HINT: you can notify user about success usage
		
	-- /ghost open [code] [user] [slot]
	elseif (cmd == "open") or (cmd == "close") then
		-- HINT: you can notify user about success usage 
		
	-- /ghost [cmd] [code] [user] [message]
	elseif (cmd == "start") or (cmd == "abort") 
		or (cmd == "pub") or (cmd == "priv") then
		-- HINT: you can notify user about success usage
	end
	
	-- FIXME: may be do not log commands from bot? (return -1)
	return 0
end

function gh_callback_host(username, botname, gamename)
	local gametype = "5x5"
	if string.find(game.name, "3x3") then gametype = "3x3" end
			
	gh_set_userbot(username, botname, gamename, gametype)
	api.message_send_text(username, message_type_info, nil, localize(username, "Game \"{}\" is created by {}. You are an owner.", gamename, botname))
end
function gh_callback_chost(username, botname, gamename)
	gh_set_userbot(username, botname, gamename, "")
	api.message_send_text(username, message_type_info, nil, localize(username, "Game \"{}\" is created by {}. You are an owner.", gamename, botname))
end


function gh_callback_unhost(username, gamename)
	gh_del_userbot(username)
	api.message_send_text(username, message_type_info, nil, localize(username, "The game \"{}\" was destroyed.", gamename))
end

function gh_callback_ping(username, players, pings)
	-- make sure that user is still in a game
	local account = api.account_get_by_name(username)
	local game = api.game_get_by_id(account.game_id)
	if not next(game) then return end
	-- make sure game owner is a bot
	if not gh_is_bot(game.owner) then return end

	-- get silent flag (do not send result to user)
	-- it's needed to do not show ping when user join a game, but we want to save the ping into database
	local silent = gh_get_silentflag(username)

	local users = {}

	i = 1
	-- fill usernames
	for v in string.split(players, ",") do
		-- init each user table
		users[i] = {}
		users[i]["name"] = v
		i = i + 1
	end
	i = 1
	-- fill pings
	for v in string.split(pings, ",") do
		users[i]["ping"] = v
		i = i + 1
	end
	
	local latency_all = ""
	local ms = localize(username, "ms")
	-- iterate each user
	for i,u in pairs(users) do
		-- load user pings
		local pings = account_get_botping(u["name"])
		local is_found = false
		for k,v in pairs(pings) do
			-- update user ping for current bot
			if (v.bot == game.owner) then
				pings[k].ping = u["ping"]
				is_found = true
			end
		end
		-- if bot not found in database pings then add a new row
		if not is_found then
			local item = { 
				date = os.time(), 
				bot = game.owner, 
				ping = u["ping"]
			}
			table.insert(pings, item)
		end
		
		-- save updated pings
		account_set_botping(u["name"], pings)

		latency_all = latency_all .. string.format("%s: [%s %s]; ", u["name"], u["ping"], ms)
		
		-- if game started send ping of each players on a new line
		if (game.status == game_status_started) and not (silent) then
			api.message_send_text(username, message_type_info, nil, localize(username, "{}'s latency to {}: {} {}", u["name"], game.owner, u["ping"], ms))
		end	
	end
	
	-- if game not started send pings of all players in a single line
	if not (game.status == game_status_started) and not (silent) then
		api.message_send_text(username, message_type_info, nil, localize(username, "Latency: {}", latency_all))
	end
end


function gh_callback_gameresult(players, ratings, results)
	local users = {}

	-- fill table users
	i = 1
	for v in string.split(players, ",") do
		-- init each user table
		users[i] = {}
		users[i]["name"] = v
		i = i + 1
	end
	i = 1
	for v in string.split(ratings, ",") do
		users[i]["rating"] = tonumber(v)
		i = i + 1
	end
	i = 1
	for v in string.split(results, ",") do
		users[i]["result"] = tonumber(v)
		i = i + 1
	end

	-- iterate each user
	for i,u in pairs(users) do
		local rating, wins, losses, leaves, streaks = 0,0,0,0,0
		local gametype = ""
		
		-- get stats
		if #users == 6 then
			gametype = "3x3"
			rating = account_get_dotarating_3x3(u["name"])
			wins = account_get_dotawins_3x3(u["name"])
			losses = account_get_dotalosses_3x3(u["name"])
			leaves = account_get_dotaleaves_3x3(u["name"])
			streaks = account_get_dotastreaks_3x3(u["name"])
		elseif #users == 10 then
			gametype = "5x5"
			rating = account_get_dotarating_5x5(u["name"])
			wins = account_get_dotawins_5x5(u["name"])
			losses = account_get_dotalosses_5x5(u["name"])
			leaves = account_get_dotaleaves_5x5(u["name"])
			streaks = account_get_dotastreaks_5x5(u["name"])
		else
			-- bad result
			ERROR(string.format("Bad game result (%s %s %s)", players, ratings, results))
			return
		end

		-- calc new stats
		if u["result"] == 2 then -- leave & win
			leaves = leaves + 1
			wins = wins + 1
		elseif u["result"] == 1 then -- win
			wins = wins + 1
		elseif u["result"] == 0 then -- loss
			losses = losses + 1
		elseif u["result"] == -1 then -- leave & loss
			leaves = leaves + 1
			losses = losses + 1
		end
		
		local result_str = localize(u["name"], "won")
		
		if streaks >= 0 then -- win streak
			if u["result"] > 0 then 
				streaks = streaks + 1
			else
				streaks = -1
			end
		else -- loss streak
			if u["result"] <= 0 then 
				streaks = streaks - 1
			else
				streaks = 1
			end
			result_str = localize(u["name"], "lose")
		end
		
	-- update stats
		if #users == 6 then
			account_set_dotarating_3x3(u["name"], u["rating"])
			account_set_dotawins_3x3(u["name"], wins)
			account_set_dotalosses_3x3(u["name"], losses)
			account_set_dotaleaves_3x3(u["name"], leaves)
			account_set_dotastreaks_3x3(u["name"], streaks)
		elseif #users == 10 then
			account_set_dotarating_5x5(u["name"], u["rating"])
			account_set_dotawins_5x5(u["name"], wins)
			account_set_dotalosses_5x5(u["name"], losses)
			account_set_dotaleaves_5x5(u["name"], leaves)
			account_set_dotastreaks_5x5(u["name"], streaks)
		end
		
		-- diff between old and new rating
		local points = u["rating"]-rating
		
		-- inform user about new stats
		api.message_send_text(u["name"], message_type_info, nil, localize(u["name"], "You {} {} points.", result_str, points))
		api.message_send_text(u["name"], message_type_info, nil, localize(u["name"], "New DotA ({}) rating: {} pts. Current {} streak: {}", gametype, rating, result_str, streaks))
	end
	

end
