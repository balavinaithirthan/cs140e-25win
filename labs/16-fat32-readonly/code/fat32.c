#include "rpi.h"
#include "fat32.h"
#include "fat32-helpers.h"
#include "pi-sd.h"

// Print extra tracing info when this is enabled.  You can and should add your
// own.
static int trace_p = 1;
static int init_p = 0;

fat32_boot_sec_t boot_sector;

fat32_fs_t fat32_mk(mbr_partition_ent_t *partition)
{
    demand(!init_p, "the fat32 module is already in use\n");
    // TODO: Read the boot sector (of the partition) off the SD card.
    fat32_boot_sec_t *boot_data = (fat32_boot_sec_t *)pi_sec_read(partition->lba_start, 1);

    // TODO: Verify the boot sector (also called the volume id, `fat32_volume_id_check`)
    fat32_volume_id_check(boot_data);
    boot_sector = *boot_data;
    // unimplemented();

    // TODO: Read the FS info sector (the sector immediately following the boot
    // sector) and check it (`fat32_fsinfo_check`, `fat32_fsinfo_print`)
    assert(boot_sector.info_sec_num == 1);
    struct fsinfo *fs_data = (struct fsinfo *)pi_sec_read(partition->lba_start + 1, 1);
    fat32_fsinfo_check(fs_data);
    fat32_volume_id_print("", boot_data);
    fat32_fsinfo_print("", fs_data);
    // END OF PART 2
    // The rest of this is for Part 3:

    // TODO: calculate the fat32_fs_t metadata, which we'll need to return.
    // unsigned fat_begin_lba = partition->lba_start + boot_data->reserved_area_nsec;
    // unsigned fat_num_clusters = boot_data->nsec_per_fat;

    unsigned lba_start = partition->lba_start;  // from the partition
    unsigned fat_begin_lba =
        partition->lba_start +
        boot_data->reserved_area_nsec;  // the start LBA + the number of reserved sectors
    unsigned cluster_begin_lba =
        partition->lba_start + (boot_data->nfats * boot_data->nsec_per_fat) +
        boot_data->reserved_area_nsec;                       // the beginning of the FAT, plus the
                                                             // combined length of all the FATs
    unsigned sec_per_cluster = boot_data->sec_per_cluster;   // from the boot sector
    unsigned root_first_cluster = boot_data->first_cluster;  // from the boot sector
    unsigned n_entries = boot_data->nsec_per_fat;            // from the boot sector
    // unimplemented();

    /*
     * TODO: Read in the entire fat (one copy: worth reading in the second and
     * comparing).
     *
     * The disk is divided into clusters. The number of sectors per
     * cluster is given in the boot sector byte 13. <sec_per_cluster>
     *
     * The File Allocation Table has one entry per cluster. This entry
     * uses 12, 16 or 28 bits for FAT12, FAT16 and FAT32.
     *
     * Store the FAT in a heap-allocated array.
     */
    uint32_t *fat = (uint32_t *)pi_sec_read(fat_begin_lba, n_entries);  // contains actual fat data
    uint32_t *fat2 = (uint32_t *)pi_sec_read(fat_begin_lba + n_entries, n_entries);
    for (int i = 0; i < n_entries; i++)
    {
        if (fat[i] != fat2[i])
        {
            panic("FATs are not the same\n");
        }
    }
    // unimplemented();

    // Create the FAT32 FS struct with all the metadata
    fat32_fs_t fs = (fat32_fs_t){
        .lba_start = lba_start,
        .fat_begin_lba = fat_begin_lba,
        .cluster_begin_lba = cluster_begin_lba,
        .sectors_per_cluster = sec_per_cluster,
        .root_dir_first_cluster = root_first_cluster,
        .fat = fat,
        .n_entries = n_entries,
    };

    if (trace_p)
    {
        trace("begin lba = %d\n", fs.fat_begin_lba);
        trace("cluster begin lba = %d\n", fs.cluster_begin_lba);
        trace("sectors per cluster = %d\n", fs.sectors_per_cluster);
        trace("root dir first cluster = %d\n", fs.root_dir_first_cluster);
    }

    init_p = 1;
    return fs;
}

// Given cluster_number, get lba.  Helper function.
static uint32_t cluster_to_lba(fat32_fs_t *f, uint32_t cluster_num)
{
    assert(cluster_num >= 2);
    unsigned lba = f->cluster_begin_lba + (cluster_num - 2) * f->sectors_per_cluster;
    if (trace_p) trace("cluster %d to lba: %d\n", cluster_num, lba);
    return lba;
}

pi_dirent_t fat32_get_root(fat32_fs_t *fs)
{
    // demand(init_p, "fat32 not initialized!");
    //  TODO: return the information corresponding to the root directory (just
    //  cluster_id, in this case)
    uint32_t cluster_id = fs->root_dir_first_cluster;
    return (pi_dirent_t){
        .name = "",
        .raw_name = "",
        .cluster_id = cluster_id,  // fix this
        .is_dir_p = 1,
        .nbytes = 0,
    };
}

// Given the starting cluster index, get the length of the chain.  Helper
// function.
static uint32_t get_cluster_chain_length(fat32_fs_t *fs, uint32_t start_cluster)
{
    // TODO: Walk the cluster chain in the FAT until you see a cluster where
    // `fat32_fat_entry_type(cluster) == LAST_CLUSTER`.  Count the number of
    // clusters.
    uint32_t cluster = start_cluster;
    uint32_t *fatRep = fs->fat;
    uint32_t length = 0;
    while (1)
    {
        length += 1;
        uint32_t next_cluster = fatRep[cluster];
        if (fat32_fat_entry_type(next_cluster) == LAST_CLUSTER)
        {
            break;
        }
        cluster = next_cluster;
    }
    trace("cluster chain length: %d\n", length);
    return length;
}

// Given the starting cluster index, read a cluster chain into a contiguous
// buffer.  Assume the provided buffer is large enough for the whole chain.
// Helper function.
static void read_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data)
{
    // TODO: Walk the cluster chain in the FAT until you see a cluster where
    // fat32_fat_entry_type(cluster) == LAST_CLUSTER.  For each cluster, copy it
    // to the buffer (`data`).  Be sure to offset your data pointer by the
    // appropriate amount each time.

    uint32_t cluster = start_cluster;
    uint32_t *fatRep = fs->fat;
    uint32_t i = 0;
    while (1)
    {
        /////
        // cluster lba = cluster_begin + (cluster_number - 2) * sectors_per_cluster
        unsigned cluster_lba = cluster_to_lba(fs, cluster);
        trace("cluster %u lba: %u\n", cluster, cluster_lba);
        unsigned data_offset = (fs->sectors_per_cluster * 512) * i;
        trace("data offset: %d\n", data_offset);
        pi_sd_read(data + data_offset, cluster_lba, fs->sectors_per_cluster);

        uint32_t next_cluster = fatRep[cluster];
        if (fat32_fat_entry_type(next_cluster) == LAST_CLUSTER)
        {
            break;
        }

        cluster = next_cluster;
        i++;
    }
}
// Converts a fat32 internal dirent into a generic one suitable for use outside
// this driver.
static pi_dirent_t dirent_convert(fat32_dirent_t *d)
{
    pi_dirent_t e = {
        .cluster_id = fat32_cluster_id(d),
        .is_dir_p = d->attr == FAT32_DIR,
        .nbytes = d->file_nbytes,
    };
    // can compare this name
    memcpy(e.raw_name, d->filename, sizeof d->filename);
    // for printing.
    fat32_dirent_name(d, e.name);
    return e;
}

// Gets all the dirents of a directory which starts at cluster `cluster_start`.
// Return a heap-allocated array of dirents.
static fat32_dirent_t *get_dirents(fat32_fs_t *fs, uint32_t cluster_start, uint32_t *dir_n)
{
    trace("start cluster is %d\n", cluster_start);
    trace("fs location is %x\n", fs);
    // TODO: figure out the length of the cluster chain (see
    // `get_cluster_chain_length`)
    int numClusters = get_cluster_chain_length(fs, cluster_start);
    *dir_n = (numClusters * fs->sectors_per_cluster * 512) / sizeof(fat32_dirent_t);
    trace("size of fat32_dirent_t is %d \n", sizeof(fat32_dirent_t));
    // TODO: allocate a buffer large enough to hold the whole directory
    void *directory = kmalloc(numClusters * fs->sectors_per_cluster * 512);
    trace("directory malloced: %d\n", numClusters * fs->sectors_per_cluster * 512);

    // TODO: read in the whole directory (see `read_cluster_chain`)
    read_cluster_chain(fs, cluster_start, directory);
    return (fat32_dirent_t *)directory;
}

pi_directory_t fat32_readdir(fat32_fs_t *fs, pi_dirent_t *dirent)
{
    // demand(init_p, "fat32 not initialized!");
    // demand(dirent->is_dir_p, "tried to readdir a file!");
    // TODO: use `get_dirents` to read the raw dirent structures from the disk
    uint32_t n_dirents;
    fat32_dirent_t *dirents = get_dirents(fs, dirent->cluster_id, &n_dirents);
    // for (int i = 0; i < n_dirents; i++)
    // {
    //     printk("dirents %s \n", dirents[i].filename);
    // }
    // printk("n_dirents: %d\n", n_dirents);
    // TODO: allocate space to store the pi_dirent_t return values
    pi_dirent_t *valid_directories = kmalloc(n_dirents * sizeof(pi_dirent_t));

    // TODO: iterate over the directory and create pi_dirent_ts for every valid
    // file.  Don't include empty dirents, LFNs, or Volume IDs.  You can use
    // `dirent_convert`.
    int valid = 0;
    for (int i = 0; i < n_dirents; i++)
    {
        if (fat32_dirent_free(&dirents[i])) continue;
        if (fat32_dirent_is_lfn(&dirents[i])) continue;
        if (dirents[i].attr & FAT32_VOLUME_LABEL) continue;
        valid_directories[valid] = dirent_convert(&dirents[i]);
        valid++;
    }
    printk("valid number of files %d \n", valid);

    // TODO: create a pi_directory_t using the dirents and the number of valid
    // dirents we found
    return (pi_directory_t){
        .dirents = valid_directories,
        .ndirents = valid,
    };
}

static int find_dirent_with_name(fat32_dirent_t *dirents, int num_entries, char *filename)
{
    // TODO: iterate through the dirents, looking for a file which matches the
    // name; use `fat32_dirent_name` to convert the internal name format to a
    // normal string.
    for (int i = 0; i < num_entries; i++)
    {
        pi_dirent_t pi_dirent = dirent_convert(&dirents[i]);
        if (strcmp(pi_dirent.name, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

pi_dirent_t *fat32_stat(fat32_fs_t *fs, pi_dirent_t *directory, char *filename)
{
    demand(init_p, "fat32 not initialized!");
    demand(directory->is_dir_p, "tried to use a file as a directory");

    // TODO: use `get_dirents` to read the raw dirent structures from the disk
    uint32_t num_entries;
    fat32_dirent_t *fat_directory_entries = get_dirents(fs, directory->cluster_id, &num_entries);

    // TODO: Iterate through the directory's entries and find a dirent with the
    // provided name.  Return NULL if no such dirent exists.  You can use
    // `find_dirent_with_name` if you've implemented it.
    // unimplemented();
    int dir_loc = find_dirent_with_name(fat_directory_entries, num_entries, filename);
    if (dir_loc == -1)
    {
        return NULL;
    }
    // TODO: allocate enough space for the dirent, then convert
    pi_dirent_t *dirent = kmalloc(sizeof(pi_dirent_t));
    // (`dirent_convert`) the fat32 dirent into a Pi dirent.
    *dirent = dirent_convert(&fat_directory_entries[dir_loc]);
    return dirent;
}

pi_file_t *fat32_read(fat32_fs_t *fs, pi_dirent_t *directory, char *filename)
{
    // This should be pretty similar to readdir, but simpler.
    demand(init_p, "fat32 not initialized!");
    demand(directory->is_dir_p, "tried to use a file as a directory!");

    // TODO: read the dirents of the provided directory and look for one matching the provided name
    pi_dirent_t *found = fat32_stat(fs, directory, filename);
    if (found == NULL)
    {
        return NULL;
    }

    // TODO: figure out the length of the cluster chain
    int num_clusters = get_cluster_chain_length(fs, found->cluster_id);

    // TODO: allocate a buffer large enough to hold the whole file
    // unimplemented();
    uint32_t toAlloc = num_clusters * fs->sectors_per_cluster * 512;
    void *data = kmalloc(toAlloc);

    // TODO: read in the whole file (if it's not empty)
    // unimplemented();
    if (found->nbytes > 0)
    {
        read_cluster_chain(fs, found->cluster_id, data);
    }

    // TODO: fill the pi_file_t
    pi_file_t *file = kmalloc(sizeof(pi_file_t));
    *file = (pi_file_t){
        .data = data,
        .n_data = found->nbytes,
        .n_alloc = toAlloc,
    };
    return file;
}

/******************************************************************************
 * Everything below here is for writing to the SD card (Part 7/Extension).  If
 * you're working on read-only code, you don't need any of this.
 ******************************************************************************/

static uint32_t find_free_cluster(fat32_fs_t *fs, uint32_t start_cluster)
{
    // TODO: loop through the entries in the FAT until you find a free one
    // (fat32_fat_entry_type == FREE_CLUSTER).  Start from cluster 3.  Panic if
    // there are none left.
    if (start_cluster < 3) start_cluster = 3;
    unimplemented();
    if (trace_p) trace("failed to find free cluster from %d\n", start_cluster);
    panic("No more clusters on the disk!\n");
}

static void write_fat_to_disk(fat32_fs_t *fs)
{
    // TODO: Write the FAT to disk.  In theory we should update every copy of the
    // FAT, but the first one is probably good enough.  A good OS would warn you
    // if the FATs are out of sync, but most OSes just read the first one without
    // complaining.
    if (trace_p) trace("syncing FAT\n");
    unimplemented();
}

// Given the starting cluster index, write the data in `data` over the
// pre-existing chain, adding new clusters to the end if necessary.
static void write_cluster_chain(fat32_fs_t *fs, uint32_t start_cluster, uint8_t *data,
                                uint32_t nbytes)
{
    // Walk the cluster chain in the FAT, writing the in-memory data to the
    // referenced clusters.  If the data is longer than the cluster chain, find
    // new free clusters and add them to the end of the list.
    // Things to consider:
    //  - what if the data is shorter than the cluster chain?
    //  - what if the data is longer than the cluster chain?
    //  - the last cluster needs to be marked LAST_CLUSTER
    //  - when do we want to write the updated FAT to the disk to prevent
    //  corruption?
    //  - what do we do when nbytes is 0?
    //  - what about when we don't have a valid cluster to start with?
    //
    //  This is the main "write" function we'll be using; the other functions
    //  will delegate their writing operations to this.

    // TODO: As long as we have bytes left to write and clusters to write them
    // to, walk the cluster chain writing them out.
    unimplemented();

    // TODO: If we run out of clusters to write to, find free clusters using the
    // FAT and continue writing the bytes out.  Update the FAT to reflect the new
    // cluster.
    unimplemented();

    // TODO: If we run out of bytes to write before using all the clusters, mark
    // the final cluster as "LAST_CLUSTER" in the FAT, then free all the clusters
    // later in the chain.
    unimplemented();

    // TODO: Ensure that the last cluster in the chain is marked "LAST_CLUSTER".
    // The one exception to this is if we're writing 0 bytes in total, in which
    // case we don't want to use any clusters at all.
    unimplemented();
}

int fat32_rename(fat32_fs_t *fs, pi_dirent_t *directory, char *oldname, char *newname)
{
    // TODO: Get the dirents `directory` off the disk, and iterate through them
    // looking for the file.  When you find it, rename it and write it back to
    // the disk (validate the name first).  Return 0 in case of an error, or 1
    // on success.
    // Consider:
    //  - what do you do when there's already a file with the new name?
    demand(init_p, "fat32 not initialized!");
    if (trace_p) trace("renaming %s to %s\n", oldname, newname);
    if (!fat32_is_valid_name(newname)) return 0;

    // TODO: get the dirents and find the right one
    unimplemented();

    // TODO: update the dirent's name
    unimplemented();

    // TODO: write out the directory, using the existing cluster chain (or
    // appending to the end); implementing `write_cluster_chain` will help
    unimplemented();
    return 1;
}

// Create a new directory entry for an empty file (or directory).
pi_dirent_t *fat32_create(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, int is_dir)
{
    demand(init_p, "fat32 not initialized!");
    if (trace_p) trace("creating %s\n", filename);
    if (!fat32_is_valid_name(filename)) return NULL;

    // TODO: read the dirents and make sure there isn't already a file with the
    // same name
    unimplemented();

    // TODO: look for a free directory entry and use it to store the data for the
    // new file.  If there aren't any free directory entries, either panic or
    // (better) handle it appropriately by extending the directory to a new
    // cluster.
    // When you find one, update it to match the name and attributes
    // specified; set the size and cluster to 0.
    unimplemented();

    // TODO: write out the updated directory to the disk
    unimplemented();

    // TODO: convert the dirent to a `pi_dirent_t` and return a (kmalloc'ed)
    // pointer
    unimplemented();
    pi_dirent_t *dirent = NULL;
    return dirent;
}

// Delete a file, including its directory entry.
int fat32_delete(fat32_fs_t *fs, pi_dirent_t *directory, char *filename)
{
    demand(init_p, "fat32 not initialized!");
    if (trace_p) trace("deleting %s\n", filename);
    if (!fat32_is_valid_name(filename)) return 0;
    // TODO: look for a matching directory entry, and set the first byte of the
    // name to 0xE5 to mark it as free
    unimplemented();

    // TODO: free the clusters referenced by this dirent
    unimplemented();

    // TODO: write out the updated directory to the disk
    unimplemented();
    return 0;
}

int fat32_truncate(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, unsigned length)
{
    demand(init_p, "fat32 not initialized!");
    if (trace_p) trace("truncating %s\n", filename);

    // TODO: edit the directory entry of the file to list its length as `length` bytes,
    // then modify the cluster chain to either free unused clusters or add new
    // clusters.
    //
    // Consider: what if the file we're truncating has length 0? what if we're
    // truncating to length 0?
    unimplemented();

    // TODO: write out the directory entry
    unimplemented();
    return 0;
}

int fat32_write(fat32_fs_t *fs, pi_dirent_t *directory, char *filename, pi_file_t *file)
{
    demand(init_p, "fat32 not initialized!");
    demand(directory->is_dir_p, "tried to use a file as a directory!");

    // TODO: Surprisingly, this one should be rather straightforward now.
    // - load the directory
    // - exit with an error (0) if there's no matching directory entry
    // - update the directory entry with the new size
    // - write out the file as clusters & update the FAT
    // - write out the directory entry
    // Special case: the file is empty to start with, so we need to update the
    // start cluster in the dirent

    unimplemented();
}

int fat32_flush(fat32_fs_t *fs)
{
    demand(init_p, "fat32 not initialized!");
    // no-op
    return 0;
}
