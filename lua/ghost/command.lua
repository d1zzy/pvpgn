--[[
	Connection interface between PvPGN and GHost
	https://github.com/OHSystem/ohsystem/issues/279

	
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--



------------------------------
--- USER -> PVPGN -> GHOST ---
------------------------------

-- /host [mode] [type] [gamename]
function command_host(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end

	local args = split_command(text, 3)
	if not args[1] or not args[2] or not args[3] then
		api.describe_command(account.name, args[0])
		return -1
	end
	
	-- if user already host a game
	if gh_get_userbot_name(account.name) then
		local gamename = gh_get_userbot_game(account.name)
		local game = api.game_get_by_name(gamename, account.clienttag, game_type_all)
		if next(game) then
			api.message_send_text(account.name, message_type_info, nil, localize(account.name, "You already host a game \"{}\". Use /unhost to destroy it.", gamename))
			return -1
		else
			-- if game doesn't exist then remove mapped bot for user 
			gh_del_userbot(account.name)
		end
	end
	
	-- get available bot by ping
	local botname = gh_select_bot(account.name)
	if not botname then
		api.message_send_text(account.name, message_type_error, nil, localize(account.name, "Enable to create game. HostBots are temporary offline."))
		return -1
	end
	
	-- redirect message to bot
	gh_message_send(botname, string.format("/pvpgn host %s %s %s %s", account.name, args[1], args[2], args[3]) )
	return 0
end


-- /chost [code] [gamename]
function command_chost(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end
		
	local args = split_command(text, 2)
	
	if not args[1] or not args[2] then
		api.describe_command(account.name, args[0])
		
		-- send user each map on a new line
		for i,map in pairs(gh_maplist) do
			api.message_send_text(account.name, message_type_info, nil, string.format("%s = %s", map.code, map.name) )
		end
		return -1
	end
	
	-- find map by code
	local mapfile = nil
	for i,map in pairs(gh_maplist) do
		if string.lower(args[1]) == string.lower(map.code) then mapfile = map.filename end
	end
	
	if not mapfile then
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Invalid map code.") )
		return -1
	end
	
	-- if user already host a game
	if gh_get_userbot_name(account.name) then
		local gamename = gh_get_userbot_game(account.name)
		local game = api.game_get_by_name(gamename, account.clienttag, game_type_all)
		if next(game) then
			api.message_send_text(account.name, message_type_info, nil, localize(account.name, "You already host a game \"{}\". Use /unhost to destroy it.", gamename))
			return -1
		else
			-- if game doesn't exist then remove mapped bot for user 
			gh_del_userbot(account.name)
		end
	end
	
	-- get available bot by ping
	local botname = gh_select_bot(account.name)
	if not botname then
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Enable to create game. HostBots are temporary offline."))
		return -1
	end
	
	-- redirect message to bot
	gh_message_send(botname, string.format("/pvpgn chost %s %s %s", account.name, mapfile, args[2]) )
	return 0
end

-- /unhost
function command_unhost(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end

	local botname = gh_get_userbot_name(account.name)
	-- check if user hasn't a mapped bot
	if not botname then
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "You don't host a game."))
		return -1
	end
	
	-- do not allow unhost if the game is started (and owner is a mapped user bot - 
	--  it is necessary because we can get duplicate name of another's game, 
	--  due to save/restore table state when Lua rehash)
	local game = api.game_get_by_id(account.game_id)
	if next(game) and (game.status == game_status_started) and (game.owner == botname) then
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "You can't unhost a started game."))
		return -1
	end

	-- redirect message to bot
	local botname = gh_get_userbot_name(account.name)
	gh_message_send(botname, string.format("/pvpgn unhost %s", account.name) )
	
	-- remove mapped bot anyway to make sure that it was removed 
	-- (even if bot casual shutdown before send callback)
	gh_del_userbot(account.name)
	
	api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Your game was destroyed."))

	return 0
end


-- /swap [slot1] [slot2]
function command_swap(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end
	if not gh_is_owner(account) then return 1 end
	
	local args = split_command(text, 2)
	if not args[1] or not args[2] then
		api.describe_command(account.name, args[0])
		return -1
	end
	
	-- redirect message to bot
	local botname = gh_get_userbot_name(account.name)
	gh_message_send(botname, string.format("/pvpgn swap %s %s %s", account.name, args[1], args[2]) )
	return 0
end

-- /open|close [slot]
function command_open_close(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end
	if not gh_is_owner(account) then return 1 end
	
	local args = split_command(text, 1)
	if not args[1] then
		api.describe_command(account.name, args[0])
		return -1
	end
	
	-- redirect message to bot
	local botname = gh_get_userbot_name(account.name)
	gh_message_send(botname, string.format("/pvpgn %s %s %s", args[0], account.name, args[1]) )
	return 0
end

-- /start|abort|pub|priv
function command_start_abort_pub_priv(account, text)
	if not config.ghost or not account.clienttag == CLIENTTAG_WAR3XP then return 1 end
	if not gh_is_owner(account) then return 1 end
	
	local args = split_command(text, 0)
	
	-- redirect message to bot
	local botname = gh_get_userbot_name(account.name)
	gh_message_send(botname, string.format("/pvpgn %s %s", args[0], account.name) )
	return 0
end




-- /p
function gh_command_ping(account, text)
	-- if user not in game then do not override command
	if not account.game_id then return false end
	local game = api.game_get_by_id(account.game_id)

	-- check if game owner is ghost bot
	if not gh_is_bot(game.owner) then return 1 end
	
	-- redirect message to bot
	gh_message_send(game.owner, string.format("/pvpgn ping %s", account.name))

	return 0
end

-- /stats
function gh_command_stats(account, text)

	local useracc = account
	
	local args = split_command(text, 1)
	if args[1] then
		useracc = api.account_get_by_name(args[1])
		-- if user not found
		if not next(useracc) then
			api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Invalid user."))
			return -1
		end
	end

	
	local stats = gh_get_dotastats(useracc)
	
	-- localized strings
	local win, loss = localize(account.name, "win"), localize(account.name, "loss")
	local pts = localize(account.name, "pts")

	local game = api.game_get_by_id(account.game_id)
	
	-- user who is owner of the hostbot in the current game
	local owner = gh_find_userbot_by_game(game.name)
	local gametype = "5x5"--gh_get_userbot_gametype(owner)
	
	-- user given in args or (user in game and game is ladder)
	if not args[1] and next(game) and not string:empty(gametype) then
		-- iterate all users in the game
		for u in string.split("harpywar,admin",",")  do
			-- get new stats for the player
			stats = gh_get_dotastats(api.account_get_by_name(u))
		
			local rank = stats.rank5x5
			local rating = stats.rating5x5
			local leaves = stats.leaves5x5
			local leaves_percent = stats.leaves5x5_percent
			
			-- switch gametype if game name has substr "3x3"
			if gametype == "3x3" then 
				rank = stats.rank3x3
				rating = stats.rating3x3
				leaves = stats.leaves3x3
				leaves_percent = stats.leaves3x3_percent
			end
		
			-- bnproxy stats output format (DO NOT MODIFY!!!)
			api.message_send_text(account.name, message_type_info, nil, string.format("[%s] %s DotA (%s): [%s] %d pts. Leave count: %d (%f%%)", 
				stats.country, u, gametype,
				rank, rating,
				leaves, leaves_percent))
		end

	else -- in chat
		api.message_send_text(account.name, message_type_info, nil, string.format("[%s] ", stats.country) .. localize(account.name, "{}'s record:", useracc.name))
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "DotA games") .. string.format(" (%s): %d-%d [%s] %d %s", "5x5", stats.wins5x5, stats.losses5x5, stats.rank5x5, stats.rating5x5, pts))
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "DotA games") .. string.format(" (%s): %d-%d [%s] %d %s", "3x3", stats.wins3x3, stats.losses3x3, stats.rank3x3, stats.rating3x3, pts))
		
		-- display streaks in self user stats
		if not args[1] then
			local streaks5x5_result, streaks3x3_result = win, win
			if (stats.streaks5x5 < 0) then streaks5x5_result = loss end
			if (stats.streaks3x3 < 0) then streaks3x3_result = loss end
			
			api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Current {} streak", "5x5") .. string.format(" (%s): %s", streaks5x5_result, stats.streaks5x5))
			api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Current {} streak", "3x3") .. string.format(" (%s): %s", streaks3x3_result, stats.streaks3x3))
		end
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Current leave count:") .. string.format(" %d (%s%%)", stats.leaves, stats.leaves_percent))
	end
	
	return 0
end

-- return table with all needed dota stats fields
function gh_get_dotastats(account)
	stats = {
		rating5x5 = account_get_dotarating_5x5(account.name),
		rating3x3 = account_get_dotarating_3x3(account.name),
		wins5x5 = account_get_dotawins_5x5(account.name),
		wins3x3 = account_get_dotawins_3x3(account.name),
		losses5x5 = account_get_dotalosses_5x5(account.name),
		losses3x3 = account_get_dotalosses_3x3(account.name),
		streaks5x5 = account_get_dotastreaks_5x5(account.name),
		streaks3x3 = account_get_dotastreaks_3x3(account.name),
		leaves5x5 = account_get_dotaleaves_5x5(account.name),
		leaves3x3 = account_get_dotaleaves_3x3(account.name),
		country = account.country
	}
	if not stats.country then stats.country = "-" end
	stats.country = stats.country:sub(1,2) -- two first symbols
	
	
	stats.leaves5x5_percent = math.round(stats.leaves5x5 / ((stats.wins5x5+stats.losses5x5)/100), 1)
	stats.leaves3x3_percent = math.round(stats.leaves3x3 / ((stats.wins3x3+stats.losses3x3)/100), 1)
	if math.isnan(stats.leaves5x5_percent) then stats.leaves5x5_percent = 0 end
	if math.isnan(stats.leaves3x3_percent) then stats.leaves3x3_percent = 0 end
		
	stats.leaves = stats.leaves5x5 + stats.leaves3x3
	stats.leaves_percent = math.round(stats.leaves5x5_percent + stats.leaves3x3_percent, 1)
	

	stats.rank5x5 = api.icon_get_rank(stats.rating5x5, CLIENTTAG_WAR3XP)
	stats.rank3x3 = api.icon_get_rank(stats.rating3x3, CLIENTTAG_WAR3XP)
	
	return stats
end


