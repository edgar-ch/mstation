import pytest
import binascii

import scripts.parser as parser
from scripts.exception import InputError

FAKE_BIN = binary_string = binascii.unhexlify('4d012d004703902e030b031b0c1010e17abc411166664e4112e17abcc1139a99ab41304e4082840100606eb20000a000521a')

PARSED_FAKE_BIN = {
	"MDATA_PRESSURE": 99458, 
	"MDATA_TEMP4": 21.450000762939453, 
	"MDATA_TEMP3": -23.559999465942383, 
	"MDATA_TEMP2": 12.899999618530273, 
	"MDATA_SOLAR_RADIATION": 45678, 
	"MDATA_HUMIDITY": 78, 
	"MDATA_TEMP": 23.559999465942383, 
	"MDATA_DATETIME": [46, 3, 11, 3, 27, 12, 16]}


def test_successful_get_data():
	expected_result = PARSED_FAKE_BIN
	obtained_result = parser.get_data(FAKE_BIN)

	assert expected_result == obtained_result


@pytest.mark.skip(reason="wait valid data")
def test_get_unvalid_data():

	with pytest.raises(InputError) as exinfo:
		parser.get_data(FAKE_BIN + '43gf')

	assert exinfo.msg == 'Checksums is not match'


def test_get_non_existed_data(monkeypatch):

	monkeypatch.delitem(parser.STRUCT_TYPES, name='0xa0', raising=False)

	with pytest.raises(AttributeError):
		parser.get_data(FAKE_BIN)
