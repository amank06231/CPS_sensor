// #include <zephyr/kernel.h>
// #include <zephyr/fs/fs.h>
// #include <zephyr/storage/disk_access.h>
// #include <zephyr/logging/log.h>

// LOG_MODULE_REGISTER(sd_card, LOG_LEVEL_INF);

// #define MOUNT_POINT "/SDCARD"

// static struct fs_mount_t sd_card_mount = {
//     .type = FS_FATFS,
//     .mnt_point = MOUNT_POINT,
// };

// void main(void)
// {
//         int ret;
//         struct fs_file_t file;

//         // Initialize the SPI-based disk
//         LOG_INF("Initializing SD card over SPI...");
//         ret = disk_access_init("SD");
//         if (ret)
//         {
//                 LOG_ERR("Disk access initialization failed (error: %d)", ret);
//                 return;
//         }

//         // Mount the file system
//         LOG_INF("Mounting SD card...");
//         ret = fs_mount(&sd_card_mount);
//         if (ret)
//         {
//                 LOG_ERR("Failed to mount SD card (error: %d)", ret);
//                 return;
//         }

//         LOG_INF("SD card mounted at '%s'", MOUNT_POINT);

//         // Open a file for writing
//         LOG_INF("Opening file for writing...");
//         fs_file_t_init(&file);
//         ret = fs_open(&file, MOUNT_POINT "/test.txt", FS_O_CREATE | FS_O_RDWR);
//         if (ret)
//         {
//                 LOG_ERR("Failed to open file (error: %d)", ret);
//                 return;
//         }

//         // Write data to the file
//         const char *write_data = "Hello, Zephyr SPI SD card!";
//         LOG_INF("Writing to file...");
//         ret = fs_write(&file, write_data, strlen(write_data));
//         if (ret < 0)
//         {
//                 LOG_ERR("Failed to write to file (error: %d)", ret);
//                 fs_close(&file);
//                 return;
//         }

//         fs_close(&file);
//         LOG_INF("File written successfully!");

//         // Open the file again for reading
//         LOG_INF("Opening file for reading...");
//         ret = fs_open(&file, MOUNT_POINT "/test.txt", FS_O_READ);
//         if (ret)
//         {
//                 LOG_ERR("Failed to open file (error: %d)", ret);
//                 return;
//         }

//         // Read data from the file
//         char read_data[64];
//         memset(read_data, 0, sizeof(read_data));
//         LOG_INF("Reading from file...");
//         ret = fs_read(&file, read_data, sizeof(read_data) - 1);
//         if (ret < 0)
//         {
//                 LOG_ERR("Failed to read from file (error: %d)", ret);
//                 fs_close(&file);
//                 return;
//         }

//         LOG_INF("Read data: %s", read_data);

//         fs_close(&file);

//         // Unmount the file system
//         LOG_INF("Unmounting SD card...");
//         fs_unmount(&sd_card_mount);
//         LOG_INF("SD card unmounted successfully!");
// }





#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/storage/flash_map.h>

/* GPIO led code */
//Initialization LED1 for camera capturing
#define LED1_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);

LOG_MODULE_REGISTER(main);

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/" DISK_DRIVE_NAME ":"

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,	
};

static int write_index = 1;
static int write_index_max = 64000;
static const char *disk_mount_pt = DISK_MOUNT_PT;


struct fs_file_t file;

char filename[30];

void main(void)
{	
	static const char *disk_pdrv = DISK_DRIVE_NAME;
	
	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);	

	int rc; // to store return values of functions	
	int ret;	

	k_msleep(1000);

	// camera_device = device_get_binding(DT_NODE_FULL_NAME(DT_NODELABEL(arducam_mega)));	

	while (1) {
		
		fs_file_t_init(&file);		
		if (write_index > write_index_max){
			write_index = 1;
		}

		sprintf(&filename, "/SD:/%d_01.txt", write_index);	
			
		// fs_unlink(filename);	     
		
		ret = fs_open(&file, filename, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);		
		
		// arducam_mega_take_picture(camera_device, CAM_IMAGE_MODE_FHD, CAM_IMAGE_PIX_FMT_JPG);
		gpio_pin_configure_dt(&led1, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
		gpio_pin_set_dt(&led1, 1);
		K_MSEC(100);
		char cam_buff[20]="hello SDCard";
		uint8_t bytes_read;		
		do {			
			// bytes_read =
			// 	arducam_mega_read_image_buf(camera_device, cam_buff, 128);			
			
			ret = fs_write(&file, &cam_buff, sizeof(cam_buff));

                        if(ret==0)
                        {
                                printk("succesfully write\n");
                        }

                        else printk("write failed\n");		

		} while (bytes_read > 0);

		gpio_pin_configure_dt(&led1, GPIO_OUTPUT | GPIO_ACTIVE_LOW);
		gpio_pin_set_dt(&led1, 0);
		K_MSEC(100);		
		ret = fs_close(&file);
		
		write_index++;
		
		k_msleep(1000);
	}
}
