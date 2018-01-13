# include <linux/init.h>
# include <linux/module.h>
# include <linux/kernel.h>
# include <linux/fs.h>
# include <linux/cdev.h>
# include <linux/slab.h>
# include <asm/uaccess.h>
# include <asm/io.h>

# define ioaddress_SIE_BASE 0x3F804000
# define ioaddress_SIZE 0x20
# define mpu_addr 0x68
#define I2C_C       0x0
#define I2C_S       0x4
#define I2C_DLEN    0x8
#define I2C_A       0xc
#define I2C_FIFO    0x10
#define I2C_DIV     0x14
#define I2C_DEL     0x18
#define I2C_CLKT    0x1c

MODULE_LICENSE("Dual BSD/GPL");

static char *nombre="mpu";
static unsigned int PrimerMenor=0;
static unsigned int cuenta=1;
static dev_t mpu_module_number;
static void __iomem *ioaddress_SIE;
char *buffer;
struct mpu_dev *mpu_dev_puntero;
struct mpu_dev {
 struct cdev mpu_cdev;
};

unsigned int leer(unsigned int address){
    unsigned int bandera;
    unsigned int ret;
    iowrite32(0X00000000, ioaddress_SIE+I2C_C);
    iowrite32(0X00008010,ioaddress_SIE+I2C_C);
    iowrite32(0x00000001, ioaddress_SIE+I2C_DLEN);
    iowrite32(address, ioaddress_SIE+I2C_FIFO);
    iowrite32(0X00000068, ioaddress_SIE+I2C_A);
    iowrite32(0X00008080, ioaddress_SIE+I2C_C);
    do{
        bandera=ioread32(ioaddress_SIE+I2C_S);
    }while(!(bandera & (1<<1)));
    iowrite32(0X00008010,ioaddress_SIE+I2C_C);
    iowrite32(0X00008081, ioaddress_SIE+I2C_C);
    iowrite32(0X00000002, ioaddress_SIE+I2C_S);
    do{
        bandera=ioread32(ioaddress_SIE+I2C_S);
    }while(!(bandera & (1<<1)));
    ret=ioread32(ioaddress_SIE+I2C_FIFO);
    printk(KERN_INFO "El valor leido es %X",ret);
    iowrite32(0X00000002, ioaddress_SIE+I2C_S);
    iowrite32(0X00008000, ioaddress_SIE+I2C_C);
    iowrite32(0X00008010, ioaddress_SIE+I2C_C);
    iowrite32(0X00008000, ioaddress_SIE+I2C_C);
    printk(KERN_INFO "lectura exitosa \n");
    return ret;
}

void lec_total(unsigned int data[12]){
    int i=0;
    //unsigned int data_mpu[12];
    for(i=0;i<6;i++){
        data[i]=leer(0x3B+i);
    }
    for(i=0;i<6;i++){
        data[i+6]=leer(0x43+i);
    }
}

int led_open (struct inode *puntero_inode, struct file *puntero_file){
    struct mpu_dev *mpu_dev_puntero;
    printk(KERN_INFO "led_SIE: se ha abierto el dispositivo \n");
    mpu_dev_puntero=container_of(puntero_inode->i_cdev, struct mpu_dev, mpu_cdev);
    puntero_file->private_data=mpu_dev_puntero;
    /*iowrite32(0x00000000, ioaddress_SIE);*/
    return 0;
}

int led_release (struct inode *puntero_inode, struct file *puntero_file){

    printk(KERN_INFO "led_SIE: se ha liberado el dispositivo");
    return 0;
}

static ssize_t led_read(struct file *puntero_file, char *buffer_user, size_t tamano, loff_t *puntero_offset){
    int i=0;
    unsigned int k=0;
    unsigned long prueba;
    char buffer_out[tamano];
    unsigned int data_mpu[12];

    k=leer(0X00000075);
    printk(KERN_INFO "%X\n",k);

    lec_total(data_mpu);

    for(i=0;i<12;++i){
        buffer_out[i] = (unsigned char)data_mpu[i];
        printk(KERN_INFO "el dato numero %i es %X", i, data_mpu[i]);
    }

    printk(KERN_INFO "%s\n", buffer_out);

    prueba = copy_to_user(buffer_user, buffer_out, tamano);

    return tamano;
}

static ssize_t led_write(struct file *puntero_file, const char *buffer_user, size_t tamano, loff_t *puntero_offset){    
    return tamano;
}

struct file_operations mpu_fops = {
    .owner = THIS_MODULE,
    .read  = led_read,
    .write = led_write,
    .open = led_open,
    .release = led_release
};

static int hello_init(void)
{
    int resultado, error;
    unsigned int bandera;
    printk(KERN_ALERT "Se inicio la construccion del driver\n");
    resultado=alloc_chrdev_region(&mpu_module_number, PrimerMenor, cuenta, nombre);
    if(resultado==0)
        printk(KERN_ALERT "Se reservaron los siguientes numeros \n Mayor: %d\n Menor: %d\n" ,MAJOR(mpu_module_number), MINOR    (mpu_module_number));
    
    else
        printk(KERN_ALERT "Hubo un error y los numeros no se reservaron. Error: %d\n", resultado);
    mpu_dev_puntero=kmalloc(sizeof(struct mpu_dev), GFP_KERNEL);
    buffer=kmalloc(2, GFP_KERNEL);
    cdev_init(&mpu_dev_puntero->mpu_cdev ,&mpu_fops);
    mpu_dev_puntero->mpu_cdev .owner=THIS_MODULE;
    mpu_dev_puntero->mpu_cdev .ops=&mpu_fops;
    error=cdev_add(&mpu_dev_puntero->mpu_cdev, mpu_module_number, cuenta);
    if(error)
        printk(KERN_INFO "error $d al anadir mpu_dev");
    
    ioaddress_SIE=ioremap(ioaddress_SIE_BASE, ioaddress_SIZE);
    printk(KERN_ALERT "ioaddress_SIE_BASE fue mapeado a: %p \n",ioaddress_SIE);

    iowrite32(0x00008000, ioaddress_SIE+I2C_C);
    iowrite32(0x00000002, ioaddress_SIE+I2C_DLEN);
    iowrite32(0x0000006B, ioaddress_SIE+I2C_FIFO); //Direccion de manejo de poder
    iowrite32(0x00000068, ioaddress_SIE+I2C_A);
    iowrite32(0x00008080, ioaddress_SIE+I2C_C);
    do{
        bandera=ioread32(ioaddress_SIE+I2C_S);
    }while(!(bandera & (1<<6)));
    iowrite32(0x00000000, ioaddress_SIE+I2C_FIFO); //Se inicia la operacion del mpu
    do{
        bandera=ioread32(ioaddress_SIE+I2C_S);
    }while(!(bandera & (1<<1)));
    iowrite32(0x00008000, ioaddress_SIE+I2C_C);
    iowrite32(0x00008010, ioaddress_SIE+I2C_C);
    iowrite32(0x00008000, ioaddress_SIE+I2C_C); 
    printk(KERN_INFO "Escritura exitosa \n");
    return 0;
}


static void hello_exit(void)
{
    printk(KERN_ALERT "Adios, mundo cruel\n");
    
    unregister_chrdev_region(mpu_module_number, cuenta);
    cdev_del(&mpu_dev_puntero->mpu_cdev);
    kfree(mpu_dev_puntero);
    kfree(buffer);
    iowrite32(0x00000000, ioaddress_SIE);
    iowrite32(0x00000002, ioaddress_SIE+I2C_S);
    iounmap(ioaddress_SIE);
}

module_init(hello_init);
module_exit(hello_exit);
