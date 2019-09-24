#!/usr/bin/env python3
#
# Build Actions NVRAM config binary file
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import struct
import argparse
import zlib

# private module
from nvram_prop import *;

NVRAM_WRITE_REGION_ALIGN_SIZE = 256
NVRAM_REGION_HEADER_SIZE = 24
NVRAM_ITEM_HEADER_SIZE = 12

def calc_hash(key):
    hash = 0
    for byte in key:
        hash = hash * 16777619
        hash = hash ^ byte
        hash = hash & 0xffffffff
    return hash

'''
struct nvram_item {
	uint32_t hash;
	uint16_t size;
	uint16_t reserved;
	uint16_t name_size;
	uint16_t data_size;
	char data[0];
};
'''
def gen_item(key, value):
        # append '\0' to string
        key_data = bytearray(key, 'utf8') + bytearray(1)
        key_data_len = len(key_data)
        value_data = bytearray(value, 'utf8') + bytearray(1)
        value_data_len = len(value_data)
        hash = calc_hash(key_data)
        total_len = NVRAM_ITEM_HEADER_SIZE + key_data_len + value_data_len

        # 4 byte aligned
        pad_data = bytearray(0)
        if total_len & 0x3:
             pad_len = 4 - (total_len & 0x3)
             pad_data = bytearray(pad_len)
             total_len = total_len + pad_len

        item_header = struct.pack('<IHHHH', hash, total_len, 0, key_data_len, value_data_len)
        item = item_header + key_data + value_data + pad_data

        return (item, total_len)

def build_nvram_region(filename, props):
    fr_file = open(filename,'wb+')
    fr_file.seek(0, 0)
    buildprops = props.get_all()

    # write NVRAM items
    data_len = 0;
    for key, value in buildprops.items():
        print('NVRAM: Property: ' + key + '=' + value);

        (item, item_len) = gen_item(key, value)
        data_len = data_len + item_len
        fr_file.write(item);

    # padding region aligned to NVRAM_WRITE_REGION_ALIGN_SIZE
    pos = fr_file.tell() + NVRAM_REGION_HEADER_SIZE
    pad_len = 0
    if (pos % NVRAM_WRITE_REGION_ALIGN_SIZE):
        pad_len = NVRAM_WRITE_REGION_ALIGN_SIZE - pos % NVRAM_WRITE_REGION_ALIGN_SIZE
        fr_file.write(bytearray(pad_len));

    file_len = pos + pad_len

    '''
    struct region_info
    {
        uint32_t magic;
        uint32_t age;

        uint32_t addr;
        int32_t size;
        int32_t data_size;

        uint32_t crc;
    };
    '''

    # write NVRAM region header
    fr_info = struct.pack('<IIIII', 0x4752564e, 0x1, 0, file_len, data_len)
    fr_file.write(fr_info);

    # caculate NVRAM region crc
    fr_file.seek(0, 0)
    crc = zlib.crc32(fr_file.read(file_len - 4), 0) & 0xffffffff
    fr_file.close()

    fr_file = open(filename,'ab+')
    fr_file.write(struct.pack('<I', crc))
    fr_file.close()

def main(argv):
    parser = argparse.ArgumentParser(
        description='Pack config files to NVRAM binary data',
    )
    parser.add_argument('-o', dest = 'output_file')
    parser.add_argument('input_files', nargs = '*')
    args = parser.parse_args();

    print('NVRAM: Build Factory NVRAM binary file')

    lines = []
    for input_file in args.input_files:
        if not os.path.isfile(input_file):
            continue

        print('NVRAM: Process property file: %s' %input_file)
        with open(input_file) as f:
            lines = lines + f.readlines()        

    properties = PropFile(lines)

    # write the merged property file
    properties.write(os.path.join(os.path.dirname(args.output_file), 'nvram.prop'))

    print('NVRAM: Generate NVRAM file: %s.' %args.output_file)
    build_nvram_region(args.output_file, properties)
   
if __name__ == "__main__":
    main(sys.argv)
