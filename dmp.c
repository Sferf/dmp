#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>


#define DM_MSG_PREFIX "dmp"

struct my_dmp_target {
  struct dm_dev *dev;
};

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
  struct my_dmp_target *mdt;

  // printk(KERN_CRIT "\n >>in function dmp_ctr \n");

	if (argc == 1) {
    // printk("argc %d", argc);
    // printk("argv %s", argv[0]);
	} else {
    (KERN_CRIT "\n Invalid no.of arguments.\n");
    ti->error = "Invalid argument count";
		return -EINVAL;
  }

  mdt = kmalloc(sizeof(struct my_dmp_target), GFP_KERNEL);

  if (mdt == NULL) {
    printk(KERN_CRIT "\n Mdt is null\n");
    ti->error = "dmp_target: Cannot allocate context";
    return -ENOMEM;
  }

  if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &mdt->dev)) {
    ti->error = "dm-dmp: Device lookup failed";
    goto free_mdt;
  }
  ti->private = mdt;

  // printk(KERN_CRIT "\n>>out function dmp_ctr\n");                       
  return 0;

free_mdt:
  kfree(mdt);
  // printk(KERN_CRIT "\n>>out function dmp_ctr with error \n");           
  return -EINVAL;
}

static void dmp_dtr(struct dm_target *ti)
{
  struct my_dmp_target *mdt = (struct my_dmp_target *) ti->private;
  // printk(KERN_CRIT "\n<<in function dmp_dtr \n");        
  dm_put_device(ti, mdt->dev);
  kfree(mdt);
  // printk(KERN_CRIT "\n>>out function dmp_dtr \n");               
}

struct dmp_stats_t {
  atomic64_t read_requests_count;
  atomic64_t read_blocks_sum;

  atomic64_t write_requests_count;
  atomic64_t write_blocks_sum;

};
static struct dmp_stats_t dmp_stats;

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
  // printk(KERN_CRIT "\n<<in function dmp_map \n");        
  struct my_dmp_target* mdtp = ti->private;

  int dir = bio_data_dir(bio);
  if (dir == READ) {
    // printk("\n read \n");
    atomic64_inc(&dmp_stats.read_requests_count);
    atomic64_add(bio->bi_io_vec->bv_len, &dmp_stats.read_blocks_sum);
  } else {
    // printk("\n write \n");
    atomic64_inc(&dmp_stats.write_requests_count);
    atomic64_add(bio->bi_io_vec->bv_len, &dmp_stats.write_blocks_sum);
  }
  bio_set_dev(bio, mdtp->dev->bdev);

  // printk(KERN_CRIT "\n>>out function dmp_map \n");               
	return DM_MAPIO_REMAPPED;
}


static struct target_type dmp_target = {
	.name   = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr    = dmp_ctr,
  .dtr    = dmp_dtr,
	.map    = dmp_map,
};

static ssize_t volumes_show(struct kobject *kobj,
                            struct kobj_attribute *attr,
                            char *buf)
{
  int64_t read_requests = atomic64_read(&dmp_stats.read_requests_count);
  int64_t write_requests = atomic64_read(&dmp_stats.write_requests_count);
  int64_t total_requests = read_requests + write_requests;

  int64_t read_blocks_avg = 0;
  if (read_requests != 0) {
    read_blocks_avg = 
      atomic64_read(&dmp_stats.read_blocks_sum) / read_requests;
  }
  int64_t write_blocks_avg = 0;
  if (write_requests != 0) {
    write_blocks_avg = 
      atomic64_read(&dmp_stats.write_blocks_sum) / write_requests;
  }
  int64_t total_blocks_avg = 0;
  if (total_requests != 0) {
    total_blocks_avg = 
      (read_blocks_avg * read_requests + 
      write_blocks_avg * write_requests) / total_requests;
  }
  return sprintf(buf, 
"read:\n\
  reqs: %lld\n\
  avg size: %lld\n\
write:\n\
  reqs: %lld\n\
  avg size: %lld\n\
total:\n\
  reqs: %lld\n\
  avg size: %lld\n\
",
  read_requests, 
  read_blocks_avg,
  write_requests,
  write_blocks_avg,
  total_requests,
  total_blocks_avg
);
}

struct kobj_attribute volumes_attr =
	__ATTR(volumes, 0444, volumes_show, NULL);

static struct kobject *stat_kobj;

static int __init dmp_init(void)
{ 
  int err = 0;

  atomic64_set(&dmp_stats.read_blocks_sum, 0);
  atomic64_set(&dmp_stats.read_requests_count, 0);
  atomic64_set(&dmp_stats.write_blocks_sum, 0);
  atomic64_set(&dmp_stats.write_requests_count, 0);

  stat_kobj = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
  if (stat_kobj == NULL) {
    pr_err("dmp: can not allocate kobj for stat (%d)\n", err);
    goto free_kobj;

  }

  err = sysfs_create_file(stat_kobj, &volumes_attr.attr);
  if (err) {
    pr_err("dmp: can not create sysfs stat group (%d)\n", err);
    return err;
  }

  err = dm_register_target(&(dmp_target));
  if (err) {
    pr_err("dmp: can not register target (%d)\n", err);
    goto free_sysfs;
  }

  return 0;
free_sysfs:
  sysfs_remove_file(stat_kobj, &volumes_attr.attr);

free_kobj:
  kobject_put(stat_kobj);
  return err;
}
module_init(dmp_init);

static void __exit dmp_exit(void)
{
	dm_unregister_target(&(dmp_target));
  sysfs_remove_file(stat_kobj, &volumes_attr.attr);
  kobject_put(stat_kobj);
}
module_exit(dmp_exit)

MODULE_AUTHOR("Pasha Isachenko <Isach.Pasha@yandex.by>");
MODULE_DESCRIPTION(DM_NAME " device mapper proxy for stats");
MODULE_LICENSE("GPL");
