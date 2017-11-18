#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/paravirt.h>

#define PROC_NAME "modu1"   // Name of the proc file 
#define MAX 4                 
#define pdpt1 0xc0000000    // Mask for Page Directory pointer table
#define pd1 0x3fe00000      // Mask for Page Directory table
#define pt1 0x1ff000        // Mask for the Page Table
#define off1 0xfff          // Mask for offset

static int VA;              // Variable to store the Virtual address
static uint64_t PA;         // Variable to get the contents of the page 


//static unsigned long VA_size = 0;

static struct proc_dir_entry *our_proc_file;

uint64_t read_dword_at_paddr(uint64_t paddr) {  // This function reads a dword from the given physical address
  /* PAE is enabled so the physical address increases from 32 to 36 bits.
   *    * (As a result, addressable memory increases from 4GB to 64GB).
     *       */
  phys_addr_t paddr_36 = (phys_addr_t)0x0FFFFFFFF & paddr;
  uint64_t *tmp = phys_to_virt(paddr_36);
  printk("paddr=%llx, va=%p, result=%llx\n", paddr, tmp, *tmp);
  /*printk("tmp=%p *tmp=%llx\n", tmp, *tmp);*/
  return *tmp;
} 

static ssize_t procfile_read(struct file *filp, char *buffer, size_t length, loff_t *offset ) { 
   /*    Function which gives the corresponding physical address to
        a virtual address that has been written to it. The address translation happens
            here. The arguments for the function are fixed and are defined in the kernel
            */ 
  int pdpt, pd, pt, offse;
  uint64_t base, base2, base3, base4, check; // declared as int 64 since page table entries are larger than 32 bits
  static int finished = 0;
  if(finished)  {  // So that the read function ends ( This is a standard method in the proc modules )
     printk(KERN_ALERT "procfile_read: END"); //Showing end of the read
     finished = 0;
     return 0;                                // so that the Function understands that the Read has ended
  }
  finished = 1;
  if(VA){
     printk(KERN_ALERT "The Address passed is %x",VA);
     pdpt = (VA & pdpt1) >> 30;            // The first 2 bits for the PDPT offset
     pd = (VA & pd1) >> 21;                // The next 9 bits for the PD offset
     pt = (VA & pt1) >> 12;                // The next 9 bits for the Page table offset
     offse = VA & off1;                    // The last 12 bits for the Page table address
     base = read_cr3();                    // base is a physical addresss
     printk(KERN_ALERT "CR3 = %llx", base); 
     base2 = read_dword_at_paddr(base + 8 * pdpt);   // Read the address of the value at CR3 + offset
     if(base2 == 0) {                     //  To check if the value is 0 so that the module does not cause kernel panic
       printk(KERN_ALERT  "FAILED base2 is 0");
       return 0;
     } 
     printk(KERN_ALERT "BASE2 = %llx", base2); // base 2 is the page Directory table base address
     base3 = read_dword_at_paddr( ( base2 & 0xfffff000 ) + 8 * pd);         // Read the address of the value at PD + offset
     printk(KERN_ALERT "Value of BASE3=%llx",base3);    // base 3 is the page table base address
     if(base3 == 0) {                         
       printk(KERN_ALERT  "FAILED base3 is 0");
       return 0;
     } 
     //printk(KERN_ALERT "Base3 = %llx", base3);           
     base4 = read_dword_at_paddr( ( base3 & 0xfffff000 ) + 8 * pt);         // Read the address of the value at pt + offset
     if (base4 == 0)  {
       printk(KERN_ALERT  "FAILED base4 is 0");
       return 0;
     }
     printk(KERN_ALERT "Base4: %llx", base4); // base 4 is the physical address we had to get :)
     PA = read_dword_at_paddr( ( base4 & 0xfffff000 ) + 8 * offse);        // Read the contents of the corresponding page
     printk(KERN_ALERT "Final: %llx", PA);     // Should have the contents of the page 
      if (copy_to_user(buffer, &PA, MAX)) {
        	printk(KERN_ALERT "Nothing to show folks");
        	return -EFAULT;
     }
     printk(KERN_ALERT "The memory address has %llx", PA); 
     return MAX;
      } 
    printk(KERN_ALERT "Enter a proper memory address");
    return 0;
}

static ssize_t procfile_write(struct file *file, const char *buffer, size_t count, loff_t *offset) { //function is defined in 
// kernel that is the reason for the arguments
   if(copy_from_user(&VA, buffer, MAX)) {
	 printk(KERN_ALERT "Write Failed");
     return -EFAULT;
    } // copying from userspace and storing it into VA
   printk(KERN_ALERT "Virtual Address stored : %x", VA);
   return MAX;
} 

static  struct file_operations file_ops =  {
    .read = procfile_read,
    .write = procfile_write,
};

int init_module() {  //gets executed when the module is inserted
    our_proc_file = proc_create(PROC_NAME, 0644, NULL, &file_ops); //Creating the proc file
    if(our_proc_file  == NULL) {
      printk(KERN_ALERT "Error");
      return -ENOMEM;
    }   // checked for errors
    printk(KERN_ALERT "%s is ready\n",PROC_NAME);
    return 0;
}

void cleanup_module( ) {  // should be cleaning up everything .. executed when the modules is cleaned up 
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_ALERT "%s has been removed\n", PROC_NAME);
}
