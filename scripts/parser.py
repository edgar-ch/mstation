import logging

from construct import Struct, Int8ul, Int16ul, Int32ul, Array, Float32l, Byte
import binascii

from exception import InputError


logger = logging.getLogger(name='parsing')


# type of structures in body in 'mdata_packet', value is structs names and zizes
STRUCT_TYPES = {
     "0x10": ('MDATA_TEMP', Float32l),
     "0x11": ('MDATA_TEMP2', Float32l),
     "0x12": ('MDATA_TEMP3', Float32l),
     "0x13": ('MDATA_TEMP4', Float32l),
     "0x20": ('MDATA_SOIL_TEMP', ),
     "0x21": ('MDATA_SOIL_TEMP_10', ),
     "0x22": ('MDATA_SOIL_TEMP_20', ),
     "0x23": ('MDATA_SOIL_TEMP_50', ),
     "0x30": ('MDATA_HUMIDITY', Int8ul),
     "0x40": ('MDATA_PRESSURE', Int32ul),
     "0x50": ('MDATA_WIND_DIR', ),
     "0x51": ('MDATA_WIND_SPEED', ),
     "0x60": ('MDATA_SOLAR_RADIATION', Int32ul),
     "0x70": ('MDATA_SOLAR_LEN', ),
     "0x80": ('MDATA_PRECIPITATION', ),
     "0x90": ('MDATA_DATETIME', 7),
     "0xa0": ('MDATA_LUX', Float32l),
}


# Header lenth on bytes
HEADER_LENTH = 5


# Parameters for parsing header data 
HEADER_FORMAT = Struct(
    "type" / Int8ul,
    "version" / Int8ul,
    "lenth" / Int8ul,
    "checksum" / Int16ul,
)


#TODO -- switch to network reading
def get_binary_data(origin):
    with open(origin, 'rb') as f:
        data = f.read()
    return data


def get_data(binary_data):
    """Get data from binary string."""

    raw_header, raw_body = binary_data[:HEADER_LENTH], binary_data[HEADER_LENTH+1:]

    # TODO !!! Uncomment when will take valid data !!!
    #header = HEADER_FORMAT.parse(raw_header)
    # if binascii.crc_hqx(raw_body, 0xffff) != header.checksum:
    #     logger.exception('Checksums is not match')
    #     raise InputError(binascii.crc_hqx(raw_body, 0xffff), 'Checksums is not match')

    result = {}

    while True:
        current_type = raw_body[:1]
        if not current_type:
            return result
        type_of_struct = "0x{}".format(binascii.hexlify(current_type))

        data = STRUCT_TYPES.get(type_of_struct, None)
        if not data:
            logger.exception('Data type is not exist')
            raise AttributeError

        data_size = data[1] if not isinstance(data[1], int) else Byte[data[1]]
        body_format = Struct(
            "type" / Int8ul,
            "data" / data_size,
        )

        # TODO !!! Remove that exception when will take valid data
        try:
            current = body_format.parse(raw_body[:data_size.sizeof()+1])
        except :
            return result

        result[data[0]] =  current.data

        raw_body = raw_body[body_format.sizeof():]
