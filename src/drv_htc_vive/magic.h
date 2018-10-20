// Copyright 2013, Fredrik Hultin.
// Copyright 2013, Jakob Bornecrantz.
// Copyright 2013, Joey Ferwerda.
// SPDX-License-Identifier: BSL-1.0
/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 */

/* HTC Vive Driver */


static const unsigned char vive_magic_power_on[64] = {
	0x04, 0x78, 0x29, 0x38, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
	0xa8, 0x0d, 0x76, 0x00, 0x40, 0xfc, 0x01, 0x05, 0xfa, 0xec, 0xd1, 0x6d, 0x00,
	0x00, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa8, 0x0d, 0x76, 0x00, 0x68, 0xfc,
	0x01, 0x05, 0x2c, 0xb0, 0x2e, 0x65, 0x7a, 0x0d, 0x76, 0x00, 0x68, 0x54, 0x72,
	0x00, 0x18, 0x54, 0x72, 0x00, 0x00, 0x6a, 0x72, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char vive_magic_power_off1[64] = {
	0x04, 0x78, 0x29, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	0x30, 0x05, 0x77, 0x00, 0x30, 0x05, 0x77, 0x00, 0x6c, 0x4d, 0x37, 0x65, 0x40,
	0xf9, 0x33, 0x00, 0x04, 0xf8, 0xa3, 0x04, 0x04, 0x00, 0x00, 0x00, 0x70, 0xb0,
	0x72, 0x00, 0xf4, 0xf7, 0xa3, 0x04, 0x7c, 0xf8, 0x33, 0x00, 0x0c, 0xf8, 0xa3,
	0x04, 0x0a, 0x6e, 0x29, 0x65, 0x24, 0xf9, 0x33, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char vive_magic_power_off2[64] = {
	0x04, 0x78, 0x29, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	0x30, 0x05, 0x77, 0x00, 0xe4, 0xf7, 0x33, 0x00, 0xe4, 0xf7, 0x33, 0x00, 0x60,
	0x6e, 0x72, 0x00, 0xb4, 0xf7, 0x33, 0x00, 0x04, 0x00, 0x00, 0x00, 0x70, 0xb0,
	0x72, 0x00, 0x90, 0xf7, 0x33, 0x00, 0x7c, 0xf8, 0x33, 0x00, 0xd0, 0xf7, 0x33,
	0x00, 0x3c, 0x68, 0x29, 0x65, 0x24, 0xf9, 0x33, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char vive_magic_enable_lighthouse[5] = {
	0x04
};
