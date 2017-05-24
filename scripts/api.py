import json

from parser import get_binary_data, get_data


# FAke origin with binary data
ORIGIN = 'data1_b.hex'


def push_to_server():
	raw_data = get_binary_data(ORIGIN)
	data = get_data(raw_data)

	return json.dumps(data)

