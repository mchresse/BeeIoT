FILENAME_BUILDNO = 'versioning'
FILENAME_VERSION_H = 'include/version.h'
vmajor = '3'
vminor = '0'

import datetime

build_no = 0
try:
    with open(FILENAME_BUILDNO) as f:
        build_no = int(f.readline()) + 1
except:
    print('Starting build number from 1..')
    build_no = 1
with open(FILENAME_BUILDNO, 'w+') as f:
    f.write(str(build_no))
    print('Build number: {}.{}.{}'.format(vmajor, vminor, build_no))

hf = """
#ifndef VMAJOR
  #define VMAJOR "{}"
#endif
#ifndef VMINOR
  #define VMINOR "{}"
#endif
#ifndef BUILD_NUMBER
  #define VBUILD "{}"
#endif
#ifndef VERSION
  #define VERSION "v{}.{}.{} - {}"
#endif
#ifndef VERSION_SHORT
  #define VERSION_SHORT "v{}.{}.{}"
#endif
""".format( vmajor,
			vminor,
			build_no,
			vmajor, vminor, build_no, datetime.datetime.now(),
			vmajor, vminor, build_no)
with open(FILENAME_VERSION_H, 'w+') as f:
    f.write(hf)