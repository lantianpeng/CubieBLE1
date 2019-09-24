#!/usr/bin/env python3
#
# Build Actions SoC firmware (RAW/USB/OTA)
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import time
import struct
import argparse
import platform
import subprocess
import array
import hashlib
import shutil
import zipfile
import xml.etree.ElementTree as ET
import zlib

from ctypes import *

# private module
from nvram_prop import *;

PARTITION_ALIGNMENT   = 0x1000

class PARTITION_ENTRY(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("name",            c_uint8 * 8),
        ("type",            c_uint16),
        ("flag",            c_uint16),
        ("offset",       c_uint32),
#        ("size",        c_uint32),
        ("seq",        c_uint16),
        ("reserve",        c_uint16),
        ("entry_offs",      c_uint32),
        ] # 64 bytes
SIZEOF_PARTITION_ENTRY   = 0x18

class PARTITION_TABLE(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("version",         c_uint16),
        ("table_size",      c_uint16),        
        ("part_cnt",        c_uint16),
        ("part_entry_size", c_uint16),
        ("reserved1",       c_uint8 * 4),
        ("parts",           PARTITION_ENTRY * 15),
        ("reserved2",       c_uint8 * 4),
        ("table_crc",       c_uint32),
        ] # 64 bytes
SIZEOF_PARTITION_TABLE   = 0x180
PARTITION_TABLE_MAGIC = 0x54504341	 #'ACPT'


partition_type_table = {'RESERVED':0, 'BOOT':1, 'SYSTEM':2, 'RECOVERY':3, 'DATA':4, 'DTM':5}

script_path = os.path.split(os.path.realpath(__file__))[0]

# table for calculating CRC
CRC16_TABLE = [
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
        ]

def reflect(crc):
    """
    :type n: int
    :rtype: int
    """
    m = ['0' for i in range(16)]
    b = bin(crc)[2:]
    m[:len(b)] = b[::-1]
    return int(''.join(m) ,2)

def _crc16(data, crc, table):
    """Calculate CRC16 using the given table.
    `data`      - data for calculating CRC, must be bytes
    `crc`       - initial value
    `table`     - table for caclulating CRC (list of 256 integers)
    Return calculated value of CRC

     polynom             :  0x1021
     order               :  16
     crcinit             :  0xffff
     crcxor              :  0x0
     refin               :  1
     refout              :  1
    """
    crc = reflect(crc)

    for byte in data:
        crc = ((crc >> 8) & 0xff) ^ table[(crc & 0xff) ^ byte]

    crc = reflect(crc)

    # swap byte
    crc = ((crc >> 8) & 0xff) | ((crc & 0xff) << 8)

    return crc

def crc16(data, crc=0xffff):
    """Calculate CRC16.
    `data`      - data for calculating CRC, must be bytes
    `crc`       - initial value
    Return calculated value of CRC
    """
    return _crc16(data, crc, CRC16_TABLE)

def crc16_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            filedata = f.read()
            crc = crc16(bytearray(filedata), 0xffff)
            return crc
    return None

def md5_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            md5 = hashlib.md5()
            md5.update(f.read())
            hash = md5.hexdigest()
            return str(hash)
    return None

def crc32_file(filename):
    if os.path.isfile(filename):
        with open(filename, 'rb') as f:
            crc = zlib.crc32(f.read(), 0) & 0xffffffff
            return crc
    return 0

def pad_file(filename, align = 4, fillbyte = 0xff):
        with open(filename, 'ab') as f:
            filesize = f.tell()
            if (filesize % align):
                padsize = align - filesize & (align - 1)
                f.write(bytearray([fillbyte]*padsize))

def new_file(filename, filesize, fillbyte = 0xff):
        with open(filename, 'wb') as f:
            f.write(bytearray([fillbyte]*filesize))

def dd_file(input_file, output_file, block_size=1, count=None, seek=None, skip=None):
    """Wrapper around the dd command"""
    cmd = [
        "dd", "if=%s" % input_file, "of=%s" % output_file,
        "bs=%s" % block_size, "conv=notrunc"]
    if count is not None:
        cmd.append("count=%s" % count)
    if seek is not None:
        cmd.append("seek=%s" % seek)
    if skip is not None:
        cmd.append("skip=%s" % skip)
    (_, exit_code) = run_cmd(cmd)

def zip_dir(source_dir, output_filename):
    zf = zipfile.ZipFile(output_filename, 'w')
    pre_len = len(os.path.dirname(source_dir))
    for parent, dirnames, filenames in os.walk(source_dir):
        for filename in filenames:
            pathfile = os.path.join(parent, filename)
            arcname = pathfile[pre_len:].strip(os.path.sep)
            zf.write(pathfile, arcname)
    zf.close()

def memcpy_n(cbuffer, bufsize, pylist):
    size = min(bufsize, len(pylist))
    for i in range(size):
        cbuffer[i]= ord(pylist[i])

def c_struct_crc(c_struct, length):
    crc_buf = (c_byte * length)()
    memmove(addressof(crc_buf), addressof(c_struct), length)
    return zlib.crc32(crc_buf, 0) & 0xffffffff

def align_down(data, alignment):
    return data & ~(alignment - 1)

def align_up(data, alignment):
    return align_down(data + alignment - 1, alignment)

def run_cmd(cmd):
    """Echo and run the given command.

    Args:
    cmd: the command represented as a list of strings.
    Returns:
    A tuple of the output and the exit code.
    """
    print("Running: ", " ".join(cmd))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = p.communicate()
    print("%s" % (output.rstrip()))
    return (output, p.returncode)

def panic(err_msg):
    print('\033[1;31;40m')
    print('FW: Error: %s\n' %err_msg)
    print('\033[0m')
    sys.exit(1)

def print_notice(msg):
    print('\033[1;32;40m%s\033[0m' %msg)

def cygpath(upath):
    cmd = ['cygpath', '-w', upath]
    (wpath, exit_code) = run_cmd(cmd)
    if (0 != exit_code):
        return upath
    return wpath.decode().strip()

def is_windows():
    sysstr = platform.system()
    if (sysstr.startswith('Windows') or \
       sysstr.startswith('MSYS') or     \
       sysstr.startswith('MINGW') or    \
       sysstr.startswith('CYGWIN')):
        return 1
    else:
        return 0

def is_msys():
    sysstr = platform.system()
    if (sysstr.startswith('MSYS') or    \
        sysstr.startswith('MINGW') or   \
        sysstr.startswith('CYGWIN')):
        return 1
    else:
        return 0

class firmware(object):
    def __init__(self, cfg_file):
        self.part_num = 0
        self.partitions = []
        self.disk_size = 0x80000   # 512KB by default
        self.fw_dir = os.path.dirname(cfg_file)
        self.bin_dir = os.path.join(self.fw_dir, 'bin')
        self.ota_dir = os.path.join(self.fw_dir, 'ota')
        self.parse_config(cfg_file)

    def parse_config(self, cfg_file):
        print('FW: Parse config file: %s' %cfg_file)
        tree = ET.ElementTree(file=cfg_file)
        root = tree.getroot()
        if (root.tag != 'firmware'):
            sys.stderr.write('error: invalid firmware config file')
            sys.exit(1)

        disk_size_prop = root.find('disk_size')
        if disk_size_prop is not None:
            self.disk_size = int(disk_size_prop.text.strip(), 0)

        part_list = root.find('partitions').findall('partition')
        for part in part_list:
            part_prop = {}
            for prop in part:
                part_prop[prop.tag] = prop.text.strip()
            self.partitions.append(part_prop)
            self.part_num = self.part_num + 1

        self.part_num = len(self.partitions);
        print(self.part_num)
        print(self.partitions)
        if (0 == self.part_num):
            panic('cannot found parition config')

        self.upate_data_hdr_checksum()
        self.generate_partition_table()
        self.generate_crc_file()
        self.caculate_part_file_max_size()
        self.check_part_file_size()

    def caculate_part_file_max_size(self):
        parts = sorted(self.partitions, key=lambda k: int(k['address'], 0))
        #print('disk size 0x%x' %(self.disk_size))
        for i in range(len(parts)):
            part_addr = int(parts[i]['address'], 0)
            if 'size' in parts[i].keys():
                parts[i]['part_file_max_size'] = int(parts[i]['size'], 0)
            else:
                if i < (len(parts) - 1):
                    next_part_addr = align_down(int(parts[i + 1]['address'], 0), PARTITION_ALIGNMENT)
                else:
                    next_part_addr = self.disk_size

                part_file_max_size = next_part_addr - part_addr
                # align with crc segment if crc is enabled
                if 'enable_crc' in parts[i].keys() and 'true' == parts[i]['enable_crc']:
                    part_file_max_size = int(part_file_max_size / 34) * 32
                
                parts[i]['part_file_max_size'] = part_file_max_size
            print('[%d]: part_addr 0x%x, max_size 0x%x' %(i, part_addr, \
                parts[i]['part_file_max_size']))

    def check_part_file_size(self):
        for part in self.partitions:
            if not 'file' in part.keys():
                continue
            partfile = os.path.join(self.bin_dir, part['file']);
            if os.path.isfile(partfile) and 'size' in part.keys():
                partfile_size = os.path.getsize(partfile)
                part_size = part['part_file_max_size']
                
                print('partition %s: \'%s\' file size 0x%x , partition size 0x%x!' \
                    %(part['name'], part['file'], partfile_size, part_size))
                if partfile_size > part_size:
                    panic('partition %s: \'%s\' file size 0x%x is bigger than partition size 0x%x!' \
                        %(part['name'], part['file'], partfile_size, part_size))

    def get_nvram_prop(self, name):
        prop_file = os.path.join(self.bin_dir, 'nvram.prop');
        if os.path.isfile(prop_file):
            with open(prop_file) as f:
                properties = PropFile(f.readlines())
                return properties.get(name)
        return ''

    '''
    def boot_calc_checksum(self, data):
            s = sum(array.array('H',data))
            s = s + 0x1234
            s = s & 0xffff
            return s

    def boot_add_cksum(self, filename):
        boot_len = os.path.getsize(filename)
        with open(filename, 'rb+') as f:
            f.seek(1024, 0)
            data = f.read(boot_len - 1024)
            checksum = self.boot_calc_checksum(data)
            f.seek(0x22, 0)
            f.write(struct.pack('<H', checksum))

            f.seek(2, 0)
            data = f.read(1022)
            checksum = self.boot_calc_checksum(data)
            f.seek(0, 0)
            f.write(struct.pack('<H', checksum))
    '''

    def calc_checksum(self, data):
            s = sum(array.array('I',data))
            return s

    def add_csum(self, filename):
        print('add_csum for %s' %(filename))
        len = os.path.getsize(filename)
        with open(filename, 'rb+') as f:
            data = f.read(0xc0)
            checksum = self.calc_checksum(data)
            f.seek(0xc0 + 0x20, 0)
            data = f.read(len - (0xc0 + 0x20))
            checksum += self.calc_checksum(data)
            checksum += 0x1234
            checksum = checksum & 0xffffffff
            f.seek(0xc0 + 0x18, 0)
            f.write(struct.pack('<I', checksum))

            f.seek(0xc0, 0)
            data = f.read(28)
            checksum = self.calc_checksum(data)
            checksum += 0x1234
            checksum = checksum & 0xffffffff
            f.seek(0xc0 + 0x1c, 0)
            f.write(struct.pack('<I', checksum))

    def upate_data_hdr_checksum(self):
        print('upate_data_hdr_checksum')
        csum_files = []
        for part in self.partitions:
            if ('BOOT' == part['type']) or ('SYSTEM' == part['type'])  or ('RECOVERY' == part['type'])  or ('DTM' == part['type']):
                csum_files.append(part['file'])

        csum_files = list(set(csum_files))
        for csum_file in csum_files:
            self.add_csum(os.path.join(self.bin_dir, csum_file))

    def generate_crc_file(self):
        crc_files = []
        for part in self.partitions:
            if ('true' == part['enable_crc']):
                crc_files.append(part['file'])

        crc_files = list(set(crc_files))
        for crc_file in crc_files:
            self.add_crc(os.path.join(self.bin_dir, crc_file))

    def generate_partition_table(self):
        part_tbl = PARTITION_TABLE()
        memset(addressof(part_tbl), 0, SIZEOF_PARTITION_TABLE)

        part_tbl.magic = PARTITION_TABLE_MAGIC
        part_tbl.ver = 0x0100
        part_tbl.table_size = SIZEOF_PARTITION_TABLE
        part_tbl.table_crc = 0x0
        part_tbl.part_entry_size = SIZEOF_PARTITION_ENTRY
        idx = 0
        for part in self.partitions:
            memcpy_n(part_tbl.parts[idx].name, 8, part['name'])
            part_tbl.parts[idx].type = partition_type_table[part['type']]

            part_tbl.parts[idx].flag = 0
            if 'enable_crc' in part.keys() and 'true' == part['enable_crc']:
                part_tbl.parts[idx].flag |= 0x1
            if 'enable_randomize' in part.keys() and 'true' == part['enable_randomize']:
                part_tbl.parts[idx].flag |= 0x2
            if 'enable_boot_check' in part.keys() and 'true' == part['enable_boot_check']:
                part_tbl.parts[idx].flag |= 0x4

            part_tbl.parts[idx].offset = align_down(int(part['address'], 0), PARTITION_ALIGNMENT)
#            part_tbl.parts[idx].size = 0
            part_tbl.parts[idx].seq = 0
            part_tbl.parts[idx].reserve = 0
            part_tbl.parts[idx].entry_offs = int(part['address'], 0)
            print('%d: 0x%x, 0x%x' %(idx, part_tbl.parts[idx].offset, part_tbl.parts[idx].seq))
            idx = idx + 1
        part_tbl.part_cnt = idx
        part_tbl.table_crc = c_struct_crc(part_tbl, SIZEOF_PARTITION_TABLE - 4)

        with open('parttbl.bin', 'wb') as f:
            f.write(part_tbl)

        boot_file = ''
        for part in self.partitions:
            if ('BOOT' == part['type']) and ('0' == part['fw_id']):
                boot_file = os.path.join(self.bin_dir, part['file']);
                break
        if boot_file != '':
            with open(boot_file, 'rb+') as f:
                f.seek(0x1000 - 0x400, 0)
                f.write(part_tbl)

    def generate_raw_image(self, rawfw_name):
        print('FW: Build raw spinor image')

        rawfw_file = os.path.join(self.fw_dir, rawfw_name)
        if os.path.exists(rawfw_file):
            os.remove(rawfw_file)

        new_file(rawfw_file, self.disk_size, 0xff)

        for part in self.partitions:
            if ('true' == part['enable_raw']):
                addr = int(part["address"], 16)
                srcfile = os.path.join(self.bin_dir, part['file'])
                print('part enable_raw, 0x%x, %s' %(addr, srcfile))
                dd_file(srcfile, rawfw_file, seek=addr)

        if not os.path.exists(rawfw_file):
            panic('Failed to generate SPINOR raw image')

    def generate_ota_files(self):
        root = ET.Element('partitions')
        root.text = '\n\t'
        root.tail = '\n'

        ota_part = 0
        for part in self.partitions:
            if ('true' == part['enable_ota']):
                part_node = ET.SubElement(root, 'partition')
                part_node.text = '\n\t\t'
                part_node.tail = '\n\t'

                type = ET.SubElement(part_node, 'type')
                type.text=part['type']
                type.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'name')
                type.text=part['name']
                type.tail = '\n\t\t'

                file = ET.SubElement(part_node, 'file')
                file.text = part['file']
                file.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'address')
                type.text=part['address']
                type.tail = '\n\t\t'

                if ('true' == part['enable_crc']):
                    type = ET.SubElement(part_node, 'enable_crc')
                    type.text=part['enable_crc']
                    type.tail = '\n\t\t'

                type = ET.SubElement(part_node, 'fw_id')
                type.text=part['fw_id']
                type.tail = '\n\t\t'

                '''
                md5 = ET.SubElement(part_node, 'md5')
                md5.text = str(md5_file(os.path.join(self.bin_dir, part['file'])))
                md5.tail = '\n\t'
                '''

                crc32= ET.SubElement(part_node, 'crc32')
                crc32.text = str(crc32_file(os.path.join(self.bin_dir, part['file'])))
                crc32.tail = '\n\t'

                shutil.copyfile(os.path.join(self.bin_dir, part['file']), \
                                os.path.join(self.ota_dir, part['file']))
                ota_part += 1

        part_num = ET.SubElement(root, 'partitionsNum')
        part_num.text=str(ota_part)
        part_num.tail = '\n\t'

        version = ET.SubElement(root, 'version')
        date_stamp = time.strftime('%y%m%d%H',time.localtime(time.time()))
        version.text=str("2.0.00." + date_stamp)
        version.tail = '\n'

        tree = ET.ElementTree(root)
        tree.write(os.path.join(self.ota_dir, 'Update.xml'), encoding='UTF-8')

    def generate_ota_image(self, ota_filename):
        print('FW: Build OTA package')

        if not os.path.isdir(self.ota_dir):
            os.mkdir(self.ota_dir)

        ota_file = os.path.join(self.fw_dir, ota_filename)
        if os.path.exists(ota_file):
            os.remove(ota_file)

        self.generate_ota_files()
        zip_dir(self.ota_dir, ota_file)

        if not os.path.exists(ota_file):
            panic('Failed to generate OTA image')

    def add_crc(self, filename):
        with open(filename, 'rb') as f:
            filedata = f.read()

        length = len(filedata)
        chunk_size = 32
        i = 0
        with open(filename, 'wb') as f:
            while (length > 0):
                if (length < chunk_size):
                    chunk = filedata[i : i + length] + bytearray(chunk_size - length)
                else:
                    chunk = filedata[i : i + chunk_size]
                crc = crc16(bytearray(chunk), 0xffff)
                f.write(chunk + struct.pack('<H',crc))
                length -= chunk_size
                i += chunk_size

def main(argv):
    parser = argparse.ArgumentParser(
        description='Build firmware',
    )
    parser.add_argument('-c', dest = 'fw_cfgfile')
    parser.add_argument('-b', dest = 'board_name')
    parser.add_argument('-ota', dest = 'build_ota')
    parser.add_argument('-raw', dest = 'build_raw')
    args = parser.parse_args();

    fw_cfgfile = args.fw_cfgfile
    board_name = args.board_name
    build_ota = 1
    if args.build_ota is not None:
        build_ota = int(args.build_ota)
    build_raw = 0
    if args.build_raw is not None:
        build_raw = int(args.build_raw)
    date_stamp = time.strftime('%y%m%d',time.localtime(time.time()))
    fw_prefix = board_name  + '_' + date_stamp

    if (not os.path.isfile(fw_cfgfile)):
        print('firmware config file is not exist')
        sys.exit(1)

    try:
        fw = firmware(fw_cfgfile)
        if build_raw != 0:
            fw.generate_raw_image(fw_prefix + '_raw.bin')
        if build_ota != 0:
            fw.generate_ota_image(fw_prefix + '_ota.zip')
        '''
        fw.generate_firmware(fw_prefix + '.fw')
        '''
    except Exception as e:
        print('\033[1;31;40m')
        print('unknown exception, %s' %(e));
        print('\033[0m')
        sys.exit(2)

if __name__ == '__main__':
    main(sys.argv[1:])
