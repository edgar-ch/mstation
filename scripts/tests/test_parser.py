import pytest

import scripts.parser as parser
from scripts.exception import InputError

FAKE_BIN = '''
	4d01 2d00 4703 902e 030b 031b 0c10 10e1
	7abc 4111 6666 4e41 12e1 7abc c113 9a99
	ab41 304e 4082 8401 0060 6eb2 0000
'''

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

	monkeypatch.delattr("parser.STRUCT_TYPES", name='0x30', raising=True)

	with pytest.raises(AttributeError):
		PARSED_FAKE_BIN.delete["MDATA_PRESSURE"]
		parser.get_data(FAKE_BIN)
