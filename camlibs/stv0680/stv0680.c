/*
 * STV0680 Vision Camera Chipset Driver
 * Copyright (C) 2000 Adam Harrison <adam@antispin.org> 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   
 */

#include <string.h>
#include <stdlib.h>

#include <gphoto2.h>
#include <gphoto2-port.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define _(String) (String)
#    define N_(String) (String)
#  endif
#else
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include "stv0680.h"
#include "library.h"

int camera_id (CameraText *id) 
{
	strcpy(id->text, "STV0680");

	return (GP_OK);
}

int camera_abilities (CameraAbilitiesList *list) 
{
	CameraAbilities a;

	strcpy(a.model, "STV0680");
	a.status = GP_DRIVER_STATUS_PRODUCTION;
	a.port     = GP_PORT_SERIAL|GP_PORT_USB;
	a.speed[0] = 115200;
	a.speed[1] = 0;
	a.operations        = GP_OPERATION_NONE;
	a.file_operations   = GP_FILE_OPERATION_PREVIEW;
	a.folder_operations = GP_FOLDER_OPERATION_NONE;

	gp_abilities_list_append(list, a);

	return (GP_OK);
}

static int camera_exit (Camera *camera) {

	struct stv0680_s *device = camera->camlib_data;

	if (device) {
		free (device);
		camera->camlib_data = NULL;
	}

	return (GP_OK);
}

static int file_list_func (CameraFilesystem *fs, const char *folder, 
			   CameraList *list, void *data) 
{
	Camera *camera = data;
	struct stv0680_s *device = camera->camlib_data;
	int count, result;

	result = stv0680_file_count(device, &count);
	if (result != GP_OK)
		return result;

	gp_list_populate(list, "image%02i.pnm", count);

	return (GP_OK);
}

static int get_file_func (CameraFilesystem *fs, const char *folder,
			  const char *filename, CameraFileType type,
			  CameraFile *file, void *user_data)
{
	Camera *camera = user_data;
	struct stv0680_s *device = camera->camlib_data;
	int image_no, result;
	char *data;
	long int size;

	image_no = gp_filesystem_number(camera->fs, folder, filename);
	if(image_no < 0)
		return image_no;

	switch (type) {
	case GP_FILE_TYPE_NORMAL:
		result = stv0680_get_image (device, image_no, &data,
					    (int*) &size);
		break;
	case GP_FILE_TYPE_PREVIEW:
		result = stv0680_get_image_preview (device, image_no, &data,
						    (int*) &size);
		break;
	default:
		return (GP_ERROR_NOT_SUPPORTED);
	}

	if (result < 0)
		return result;

	gp_file_set_name (file, filename);
	gp_file_set_mime_type (file, "image/pnm"); 
	gp_file_set_data_and_size (file, data, size);

	return (GP_OK);
}

static int camera_summary (Camera *camera, CameraText *summary) 
{
	strcpy(summary->text, _("No summary available."));

	return (GP_OK);
}

static int camera_manual (Camera *camera, CameraText *manual) 
{
	strcpy(manual->text, _("No manual available."));

	return (GP_OK);
}

static int camera_about (Camera *camera, CameraText *about) 
{
	strcpy (about->text, 
		_("STV0680\n"
		"Adam Harrison <adam@antispin.org>\n"
		"Driver for cameras using the STV0680 processor ASIC.\n"
		"Protocol reverse engineered using CommLite Beta 5"));

	return (GP_OK);
}

static const char* camera_result_as_string (Camera *camera, int result) 
{
	if (result >= 0) return ("This is not an error...");
	if (-result < 100) return gp_result_as_string (result);
	return ("This is a template specific error.");
}

int camera_init (Camera *camera) 
{
        gp_port_settings gpiod_settings;
        int ret;
        struct stv0680_s *device;

        /* First, set up all the function pointers */
        camera->functions->exit                 = camera_exit;
        camera->functions->summary              = camera_summary;
        camera->functions->manual               = camera_manual;
        camera->functions->about                = camera_about;
        camera->functions->result_as_string     = camera_result_as_string;

        if((device = malloc(sizeof(struct stv0680_s))) == NULL) {
                return GP_ERROR_NO_MEMORY;
        }

        camera->camlib_data = device;
	device->gpiod = camera->port;

	/* Configure serial port */
        gp_port_timeout_set(device->gpiod, 1000);
        strcpy(gpiod_settings.serial.port, camera->port_info->path);
        gpiod_settings.serial.speed = camera->port_info->speed;
        gpiod_settings.serial.bits = 8;
        gpiod_settings.serial.parity = 0;
        gpiod_settings.serial.stopbits = 1;
        gp_port_settings_set(device->gpiod, gpiod_settings);

	/* Set up the filesystem */
	gp_filesystem_set_list_funcs (camera->fs, file_list_func, NULL, camera);
	gp_filesystem_set_file_funcs (camera->fs, get_file_func, NULL, camera);

        /* test camera */
        ret = stv0680_ping(device);
	if (ret < 0) {
		free (device);
		camera->camlib_data = NULL;
	}

	return (ret);
}

