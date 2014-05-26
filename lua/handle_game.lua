--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- Global function to handle game create
function handle_game_create(game)
	--for i,j in pairs(game) do
	--	api.message_send_text(game.owner, message_type_info, game.owner, i.." = "..j)
	--end
end




-- Global function to handle user join to game
function handle_game_userjoin(game, account)
	--for i,j in pairs(game) do
	--	message_send_text(account.name, message_type_info, account.name, i.." = "..j)
	--end
end


-- Global function to handle user left from game
function handle_game_userleft(game, account)
	--for username in string.split(str,",")  do
	--	if (account.name ~= username) then
	--		api.message_send_text(username, message_type_whisper, nil, "Bye ".. account.name)
	--	end
    --end
end

-- Global function to handle game end
function handle_game_end(game)
	--api.message_send_text(game.owner, message_type_whisper, nil, "End game")
end

-- Global function to handle game report
function handle_game_report(game)

	--for i,j in pairs(game) do
	--	api.message_send_text("harpywar", message_type_info, game.owner, i.." = "..j)
	--	api.message_send_text(game.owner, message_type_info, game.owner, i.." = "..j)
	--end
	
	--api.eventlog(eventlog_level_gui, __FUNCTION__, game.last_access)
end



-- Global function to handle game destroy
function handle_game_destroy(game)
	--api.message_send_text(game.owner, message_type_whisper, nil, "Destroy game")
end


-- Global function to handle game status
function handle_game_changestatus(game)
	--api.message_send_text(game.owner, message_type_info, nil, "Change status of the game to ".. game.status)
end
