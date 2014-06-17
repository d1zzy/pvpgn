--[[
	Copyright (C) 2014 HarpyWar (harpywar@gmail.com)
	
	This file is a part of the PvPGN Project http://pvpgn.pro
	Licensed under the same terms as Lua itself.
]]--


-- /quiz <start|stop|stats>
function command_quiz(account, text)
	if not config.quiz then
		return 0
	end
	
	local args = split_command(text, 2)

	if (args[1] == "start") then
		return q_command_start(account, args[2])
	
	elseif (args[1] == "stop") then
		return q_command_stop(account)

	elseif (args[1] == "stats") then
		if not args[2] then
			return q_command_toplist(account)
		else
			return q_command_stats(account, args[2])
		end
	end
	
	api.describe_command(account.name, args[0])
	return 1
end


-- Start quiz in current channel
function q_command_start(account, filename)

	local channel = api.channel_get_by_id(account.channel_id)
	if not channel then
		api.message_send_text(account.name, message_type_error, account.name, "This command can only be used inside a channel.")
		return 1
	end

	if config.quiz_channel then
		api.message_send_text(account.name, message_type_error, account.name, 'Quiz has already ran in channel "'..config.quiz_channel..'". Use /qstop to force finish.')
		return 1
	end
	
	-- check if file exists
	if not filename or not file_exists(q_directory() .. "/questions/" .. filename .. ".txt") then
		api.message_send_text(account.name, message_type_error, account.name, "Available Quiz dictionaries: ")
		api.message_send_text(account.name, message_type_error, account.name, "   " .. config.quiz_filelist)
		return 1
	end

	quiz:start(channel.name, filename)
	return 1
end

-- Stop quiz
function q_command_stop(account)

	if not config.quiz_channel then
		api.message_send_text(account.name, message_type_error, account.name, 'Quiz is not running.')
		return 1
	end

	quiz:stop(account.name)

	return 1
end

-- Display Quiz Top players record
function q_command_toplist(account)
	
	-- load records (if it was not loaded yet)
	if not q_load_records() then
		return 0
	end

	local output = "Top " .. config.quiz_users_in_top .. " Quiz records:"
	api.message_send_text(account.name, message_type_info, account.name, output)

	-- display TOP of total records
	for i,t in pairs(q_records_total) do
		if (i > config.quiz_users_in_top) then break end

		local output = string.format("  %d. %s [%d points]", i, t.username, t.points)
		api.message_send_text(account.name, message_type_info, account.name, output)
	end

	return 1
end


-- Display single player's record
function q_command_stats(account, username)

	-- load records (if it was not loaded yet)
	if not q_load_records() then
		return 0
	end
	
	local found = false
	-- find user in records
	for i,t in pairs(q_records_total) do
		if string.upper(t.username) == string.upper(username) then
			api.message_send_text(account.name, message_type_info, account.name, t.username.. "'s Quiz record:")
			
			local output = string.format("  %d. %s [%d points]", i, t.username, t.points)
			api.message_send_text(account.name, message_type_info, account.name, output)
			
			found = true
		end
	end
	
	if not found then
		api.message_send_text(account.name, message_type_info, account.name, username .. " has never played Quiz.")
	end

	return 1
end
