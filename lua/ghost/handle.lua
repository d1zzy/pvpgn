--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


function gh_handle_game_userjoin(game, account)
	-- check that game owner is ghost bot
	if not gh_is_bot(game.owner) then return end
	
	-- send ping silently
	gh_set_silentflag(account.name)

	local botaccount = api.account_get_by_name(game.owner)
	-- send ping to bot (to save result)
	command_ping(botaccount, "/ping")
	
	if (config.ghost_dota_server) then
		-- user who is owner of the hostbot in the current game
		local owner = gh_find_userbot_by_game(game.name)
		-- if game is not ladder
		if string:empty(gh_get_userbot_gametype(owner)) then return end
	
		
		for u in string.split(game.players, ",") do
			-- show stats of the player who joined the game for each other player
			local useracc = api.account_get_by_name(u)
			command_stats(useracc, "/stats " .. account.name)
			
			-- show stats of each player in the game to a player who joined the game
			command_stats(account, "/stats " .. u)
		end
		api.message_send_text(account.name, message_type_info, nil, localize(account.name, "Joined ladder game. Game owner: {}", owner))
	end
end

function gh_handle_user_login(account)
	if gh_is_bot(account.name) then
		-- activate pvpgn mode on ghost side
		gh_message_send(account.name, "/pvpgn init");
		DEBUG("init bot " .. account.name)
	end
end