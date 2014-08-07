protocol = {}

protocol.message_by_id = {
	{
		name = "LOOP_SEPARATOR",
		data = {}
	},
	{
		name = "PICK_REQUEST",
		data = {}
	},
	{
		name = "ITEM_PICKED",
		data = {
			"Ushort", "subject_id",
			"Ushort", "item_id"
		}
	},
	{
		name = "ITEM_DROPPED",
		data = {
			"Ushort", "subject_id"
		}
	},
	{
		name = "DAMAGE_MESSAGE",
		data = {
			"Float", "amount",
			"Ushort", "victim_id"
		}
	},
	{
		name = "SHOT_REQUEST",
		data = {
			"Vec2", "position",
			"Float", "rotation"
		}
	},
	{
		name = "SHOT_INFO",
		data = {
			"Ushort", "delay_time",
			"Ushort", "subject_id",
			-- global space id
			"Uint", "random_seed",
			"Uint", "starting_bullet_id",
			"Vec2", "position",
			"Float", "rotation"
		}
	},
	{
		name = "HIT_REQUEST",
		data = {
			"Ushort", "victim_id",
			-- client-space id
			"Uint", "bullet_id"
		}
	},
	{
		name = "HIT_INFO",
		data = {
			"Ushort", "victim_id",
			-- global space id
			-- the sender themself doesn't need a hit confirmation
			"Uint", "bullet_id"
		}
	},
	{
		name = "INPUT_SNAPSHOT",
		data = {
			"Uint", "at_step", 
			"Bit", 	"moving_left",
			"Bit", 	"moving_right",
			"Bit", 	"moving_forward",
			"Bit", 	"moving_backward"
		}
	},
	{
		name = "CROSSHAIR_SNAPSHOT",
		data = {
			"Vec2", "position"
		}
	},
	{
		name = "CURRENT_STEP",
		data = {
			"Uint", "at_step"
		}
	},
	
	{
		name = "ASSIGN_SYNC_ID",
		data = {
			"Ushort", "sync_id"
		}
	},
	{
		name = "NEW_OBJECTS",
		data = {
			"Ushort", "bits",
			"Ushort", "object_count"
		},
		variable_size = true
	},
	{
		name = "STATE_UPDATE",
		data = {
			"Ushort", "bits",
			"Ushort", "object_count"
		},
		variable_size = true
	},
	{
		name = "DELETE_OBJECT",
		data = {
			"Ushort", "removed_id"
		}
	}
}

protocol.new_object_signature = {
	"Ushort", "id",
	"Ushort", "archetype_id"
}

-- internals
protocol.GAME_TRANSMISSION = network_event.ID_USER_PACKET_ENUM + 1

protocol.messages = {}
protocol.message_names = {}
protocol.id_by_message = {}

for i=1, #protocol.message_by_id do
	local name = protocol.message_by_id[i].name
	protocol.messages[name] = protocol.message_by_id[i]
	table.insert(protocol.message_names, name)
	protocol.id_by_message[name] = i
end	

protocol.read_var = function(var_type, in_bs)
	protocol.LAST_READ_BITSTREAM = in_bs
	
	local read_type = var_type
	if read_type == "Bool" then read_type = "Bit" end
	
	local out = in_bs["Read" .. read_type](in_bs)
	
	if var_type == "Bit" then
		out = bool2int(out)
	end
	
	return out
end

protocol.write_var = function(var_type, var, out_bs)
	if var_type == "Bit" then
		var = var > 0
	elseif var_type == "Bool" then
		-- if it's bool, it's already conditional and not an integer
		var_type = "Bit"
	end
	
	out_bs["Write" .. var_type](out_bs, var)
end

protocol.write_sig = function(sig, entry, out_bs)
	for i=1, (#sig/2) do
		local var_type = sig[i*2-1]
		local var_name = sig[i*2]
		
		out_bs:name_property(var_name)
		protocol.write_var(var_type, entry[var_name], out_bs)
	end
end

protocol.write_msg = function(name, entry)
	local out_bs = BitStream()
	out_bs:name_property(name)
	out_bs:WriteByte(protocol.id_by_message[name])
	
	local data = protocol.messages[name].data
	
	protocol.write_sig(data, entry, out_bs)
	
	return out_bs
end


protocol.read_sig = function(sig, out_entry, in_bs)
	for i=1, (#sig/2) do
		local var_type = sig[i*2-1]
		local var_name = sig[i*2]
		
		in_bs:name_property(var_name)
		out_entry[var_name] = protocol.read_var(var_type, in_bs)
	end
	
	protocol.LAST_READ_BITSTREAM = in_bs
end

protocol.read_msg = function(in_bs, out_table)
	protocol.LAST_READ_BITSTREAM = in_bs
	local out_entry = out_table
	
	if out_entry == nil then
		out_entry = {}
	end
	
	in_bs:name_property("message_type")
	local message_type = in_bs:ReadByte()
	
	out_entry.info = protocol.message_by_id[message_type]
	out_entry.data = {}
	
	local data = protocol.message_by_id[message_type].data
	
	protocol.read_sig(data, out_entry.data, in_bs)

	return out_entry
end








protocol.intent_to_name = {
	[intent_message.MOVE_FORWARD] = "forward",
	[intent_message.MOVE_BACKWARD] = "backward",
	[intent_message.MOVE_LEFT] = "left",
	[intent_message.MOVE_RIGHT] = "right"
}

protocol.name_to_command = {
	["+forward"] = 1,
	["-forward"] = 2,
	["+backward"] = 3,
	["-backward"] = 4,
	["+left"] = 5,
	["-left"] = 6,
	["+right"] = 7,
	["-right"] = 8
}

protocol.name_to_intent = {}
protocol.command_to_name = {}

for k, v in pairs (protocol.name_to_command) do
	protocol.command_to_name[v] = k
end

for k, v in pairs (protocol.intent_to_name) do
	protocol.name_to_intent[v] = k
end
