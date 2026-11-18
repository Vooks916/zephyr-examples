/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Curiosity Nano Explorer microSD card example.
 *
 * Reads the card size, mounts its FAT file system, appends one line to
 * LOG.TXT, prints the file back and lists the files in the root folder.
 * LOG.TXT grows by one line every reset, which shows that the writes
 * really reach the card.
 *
 * The card is the "SD" disk created by the zephyr,sdmmc-disk node in the
 * board overlay.
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>

#define DISK_NAME "SD"
#define MOUNT_POINT "/" DISK_NAME ":"
#define LOG_FILE MOUNT_POINT "/LOG.TXT"

static FATFS fat_fs;

static struct fs_mount_t sd_mount = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.storage_dev = (void *)DISK_NAME,
	.mnt_point = MOUNT_POINT,
	/* Never format the card, even if no FAT file system is found */
	.flags = FS_MOUNT_FLAG_NO_FORMAT,
};

static int print_card_size(void)
{
	uint32_t sector_count;
	uint32_t sector_size;
	int err;

	err = disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_INIT, NULL);
	if (err != 0) {
		printk("No card found (error %d)\n", err);
		return err;
	}

	err = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (err == 0) {
		err = disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	}
	if (err != 0) {
		printk("Could not read the card size (error %d)\n", err);
		return err;
	}

	printk("Card: %u sectors of %u bytes, %u MB\n", sector_count, sector_size,
	       (uint32_t)(((uint64_t)sector_count * sector_size) >> 20));
	return 0;
}

static int append_log_line(void)
{
	struct fs_file_t file;
	char line[48];
	int len;
	int err;

	fs_file_t_init(&file);

	err = fs_open(&file, LOG_FILE, FS_O_CREATE | FS_O_APPEND | FS_O_WRITE);
	if (err != 0) {
		printk("Could not open %s for writing (error %d)\n", LOG_FILE, err);
		return err;
	}

	len = snprintk(line, sizeof(line), "Boot at uptime %u ms\n", k_uptime_get_32());

	ssize_t written = fs_write(&file, line, len);

	err = fs_close(&file);
	if (written != len) {
		printk("Write failed (%d)\n", (int)written);
		return -EIO;
	}

	return err;
}

static int print_log(void)
{
	struct fs_file_t file;
	char buf[64];
	ssize_t len;
	int err;

	fs_file_t_init(&file);

	err = fs_open(&file, LOG_FILE, FS_O_READ);
	if (err != 0) {
		printk("Could not open %s for reading (error %d)\n", LOG_FILE, err);
		return err;
	}

	printk("--- %s ---\n", LOG_FILE);
	do {
		len = fs_read(&file, buf, sizeof(buf) - 1);
		if (len > 0) {
			buf[len] = '\0';
			printk("%s", buf);
		}
	} while (len > 0);
	printk("--- end ---\n");

	fs_close(&file);

	return (len < 0) ? (int)len : 0;
}

static int list_root(void)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int err;

	fs_dir_t_init(&dir);

	err = fs_opendir(&dir, MOUNT_POINT);
	if (err != 0) {
		printk("Could not open %s (error %d)\n", MOUNT_POINT, err);
		return err;
	}

	printk("Files in %s\n", MOUNT_POINT);
	while (true) {
		err = fs_readdir(&dir, &entry);
		if (err != 0 || entry.name[0] == '\0') {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("  [DIR]  %s\n", entry.name);
		} else {
			printk("  %8zu %s\n", entry.size, entry.name);
		}
	}

	fs_closedir(&dir);

	return err;
}

int main(void)
{
	int err;

	printk("Explorer microSD demo\n");

	if (print_card_size() != 0) {
		return 0;
	}

	err = fs_mount(&sd_mount);
	if (err != 0) {
		printk("Mount failed (error %d). Is the card FAT32 formatted?\n", err);
		return 0;
	}
	printk("Mounted at %s\n", MOUNT_POINT);

	if (append_log_line() == 0) {
		print_log();
	}

	list_root();

	err = fs_unmount(&sd_mount);
	printk("Unmounted (%d). Done.\n", err);

	return 0;
}
