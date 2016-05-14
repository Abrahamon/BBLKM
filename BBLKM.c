

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Library to control the GPIO in the beagleboard
#include <linux/interrupt.h>  // Used for the buttons interrupts
#include <linux/kobject.h>    // Kobjetcs to create a directories in systemfile
#include <linux/time.h>       // Used to time options
#include <linux/kthread.h>    // Used to create a thread to control the leds
#include <linux/delay.h>      // Used to sleep
#define  DEBOUNCE_TIME 100    //Constant

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lenin Torres & Abraham Arias");
MODULE_DESCRIPTION("BBLKM, LKM to control three leds in two modes");
MODULE_VERSION("1.0");
//


//Declaration of the GPIOs to use the pins in the beagleboard-xm
static unsigned int gpioButton = 139;       //gpiobutton = 139 to use the pin 3 in the beagleboard-xm
module_param(gpioButton, uint, S_IRUGO);    //pass the gpioButton like a parameter to the module
MODULE_PARM_DESC(gpioButton, " Button, GPIO 139, Pin 3");  ///Description of parameter to show in modinfo

static unsigned int gpioLED1 = 138;           //gpioLED1 = 138 to used the pin 5 in the beagleboard-xm
module_param(gpioLED1, uint, S_IRUGO);       //pass the gpioLED1 like a parameter to the module
MODULE_PARM_DESC(gpioLED1, " LED1, GPIO 138, Pin 5");     ///Description of parameter to show in modinfo

static unsigned int gpioLED2 = 137;           //gpioLED2 = 137 to used the pin 7 in the beagleboard-xm
module_param(gpioLED2, uint, S_IRUGO);       //pass the gpioLED2 like a parameter to the module
MODULE_PARM_DESC(gpioLED2, " LED2, GPIO 137, Pin 7");     ///Description of parameter to show in modinfo

static unsigned int gpioLED3 = 136;           //gpioLED3 = 136 to used the pin 9 in the beagleboard-xm
module_param(gpioLED3, uint, S_IRUGO);       //pass the gpioLED3 like a parameter to the module
MODULE_PARM_DESC(gpioLED3, " LED3, GPIO 136, Pin 9");     ///Description of parameter to show in modinfo


static bool isRising = 1;                   // Rising for the interrupt
module_param(isRising, bool, S_IRUGO);
MODULE_PARM_DESC(isRising, " Rising edge = 1 (default), Falling edge = 0");

static unsigned int blinkPeriod = 1000;     //Default BlinkPeriod 1000ms
module_param(blinkPeriod, uint, S_IRUGO);   // pass the blinkPeriod like a parameter to the module
MODULE_PARM_DESC(blinkPeriod, "Number in miliseconds to control the blink period of the leds");

static unsigned int burstRep = 1;     //Default burst repetition is 1
module_param(burstRep, uint, S_IRUGO);   //pass the burstRep like a parameter to the module
MODULE_PARM_DESC(burstRep, "The leds do the number burstRep of repetitions ");

static char moduleName[5] = "BBLKM";          //Module name
static int irqNumber;                    //Number
static int buttonStats = 0;            //Number of presses
static bool whiFlag = 0;                    //Flag to control the button pressed
// ledOn1,ledOn2 and ledOn3 control if the led is on or off
static bool ledOn1 = 0;
static bool ledOn2 = 0;
static bool ledOn3 = 0;
static bool LEDStatus = 0;


enum modes { ZERO,ONE, DEFAULT, BURST };     //Contains the 4 options of LEDMode ZERO = DEFAULT, ONE = BURST
static enum modes LEDMode = DEFAULT;            // DEFAULT mode is Default
static struct timespec ts_last, ts_current, ts_diff;  //get the time from time.h

//Function to control the irq handler
static irq_handler_t  BBLKM_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/*
 * The next functions are the body of the KObject, this KObject create a directory in
 * the sysfiles with all files that the module needs to work
 *
 */

static ssize_t buttonStats_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", buttonStats);
}

static ssize_t buttonStats_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count){
    sscanf(buf, "%du", &buttonStats);
    return count;
}


static ssize_t LEDStatus_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", LEDStatus);
}


//lastTime show the last time when the user press the button
static ssize_t lastTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%.2lu:%.2lu:%.2lu \n", ((ts_last.tv_sec/3600)%24),
                   (ts_last.tv_sec/60) % 60, ts_last.tv_sec % 60 );
}
//diffTime show the diference between the last two pressesin seconds
static ssize_t diffTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%lu.%.9lu\n", ts_diff.tv_sec, ts_diff.tv_nsec);
}


//This function show what is the state in the moment that the user read the file
static ssize_t LEDMode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    switch(LEDMode){
        case DEFAULT:   return sprintf(buf, "default\n");
        case ZERO:    return sprintf(buf, "0\n");
        case BURST:    return sprintf(buf, "burst\n");
        case ONE: return sprintf(buf, "1\n");
        default:    return sprintf(buf, "LKM Error\n");
    }
}

//This function store the state of the Leds
static ssize_t LEDMode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    if (strncmp(buf,"default",count-1)==0) { LEDMode = DEFAULT; }
    else if (strncmp(buf,"0",count-1)==0) { LEDMode = ZERO; }
    else if (strncmp(buf,"burst",count-1)==0) { LEDMode = BURST; }
    else if (strncmp(buf,"1",count-1)==0) { LEDMode = ONE; }
    return count;
}

/**
 * Next two functions manipulate the blinkperiod file
 * can store or read the file
 */
static ssize_t period_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", blinkPeriod);
}

static ssize_t period_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int period;                     //unsigned to make sure that the number is positive
    sscanf(buf, "%du", &period);
    blinkPeriod = period;
    return period;
}


/**
 * Next two functions manipulate the burstRep file
 * can store or read the file
 */
static ssize_t rep_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    return sprintf(buf, "%d\n", burstRep);
}

static ssize_t rep_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
    unsigned int rep;                     // Unsigned to make sure that the number is positive
    sscanf(buf, "%du", &rep);
    burstRep = rep;
    return rep;
}





/**
 * make a struct of the atributes of the Kobject
 * define what access permission have the user to the file
 */
static struct kobj_attribute period_attr = __ATTR(blinkPeriod, 0666, period_show, period_store);
static struct kobj_attribute rep_attr = __ATTR(burstRep, 0666, rep_show, rep_store);
static struct kobj_attribute LEDStatus_attr = __ATTR_RO(LEDStatus);
static struct kobj_attribute LEDMode_attr = __ATTR(LEDMode, 0666, LEDMode_show, LEDMode_store);
static struct kobj_attribute count_attr = __ATTR(buttonStats, 0666, buttonStats_show, buttonStats_store);
static struct kobj_attribute time_attr  = __ATTR_RO(lastTime);
static struct kobj_attribute diff_attr  = __ATTR_RO(diffTime);




/**
 * this make an array with all the atributtes to create a group
 */
static struct attribute *BBLKM_attrs[] = {
        &period_attr.attr,
        &rep_attr.attr,
        &LEDMode_attr.attr,
        &count_attr.attr,

        &LEDStatus_attr.attr,
        &time_attr.attr,
        &diff_attr.attr,


        NULL,
};

/** this is the atribute groups, create a directory with the name and all the files.
 */
static struct attribute_group attr_group = {
        .name  = moduleName,
        .attrs = BBLKM_attrs,
};

static struct kobject *BBLKM_kobj;            /// The pointer to the KObject
//took from the example
static struct task_struct *task;            /// The pointer to the thread task

/**
 * this is the thread, is some file change the value it can catch at the moment
 */
static int ledControl(void *arg){
    while(!kthread_should_stop()){           // Returns true when kthread_stop() is called
        set_current_state(TASK_RUNNING);
        if ((LEDMode==ZERO) | (LEDMode==DEFAULT)) { //it is the secuence to take if the state of LEDMode are  Zero or Default
            printk(KERN_INFO "BBLKM: Default mode activated\n");
			ledOn1 = whiFlag;
            ledOn2 = whiFlag;
            ledOn3 = whiFlag;
            gpio_set_value(gpioLED1, ledOn1);
            gpio_set_value(gpioLED2, ledOn2);
            gpio_set_value(gpioLED3, ledOn3);
            
        }
        else if ((LEDMode==BURST) | (LEDMode==ONE)){//it is the secuence to take if the state of LEDMode are  ONE or Burst
            printk(KERN_INFO "BBLKM: Burst mode activated\n");
            if (whiFlag==true){
                int x = burstRep;
                while(x != 0){

                    ledOn1 = !ledOn1;
                    gpio_set_value(gpioLED1, ledOn1);
                    ledOn1 = !ledOn1;
                    msleep(blinkPeriod/2);
                    gpio_set_value(gpioLED1, ledOn1);
                    ledOn2 = !ledOn2;
                    gpio_set_value(gpioLED2, ledOn2);
                    ledOn2 = !ledOn2;
                    msleep(blinkPeriod/2);
                    gpio_set_value(gpioLED2, ledOn2);
                    ledOn3 = !ledOn3;
                    gpio_set_value(gpioLED3, ledOn3);
                    ledOn3 = !ledOn3;
                    msleep(blinkPeriod/2);
                    gpio_set_value(gpioLED3, ledOn3); x=x-1;}
                whiFlag = !whiFlag;
                ledOn1=0;
                ledOn2=0;
                ledOn3=0;
            }else{
                ledOn1=0;
                ledOn2=0;
                ledOn3=0;
                gpio_set_value(gpioLED1, ledOn1);
                gpio_set_value(gpioLED2, ledOn2);
                gpio_set_value(gpioLED3, ledOn3);
            }//



        }

        set_current_state(TASK_INTERRUPTIBLE);
        msleep(blinkPeriod/2);
    }
    printk(KERN_INFO "BBLKM: Thread has run to completion \n");
    return 0;
}

/*
 *
 */
static int __init BBLKM_init(void){
    int result = 0;
    //took from example
    unsigned long IRQflags = IRQF_TRIGGER_RISING;      // The default is a rising-edge interrupt


    printk(KERN_INFO "BBLKM: Initializing the BBLKM\n");
    sprintf(moduleName, "BBLKM");


    BBLKM_kobj = kobject_create_and_add("BBLKM", kernel_kobj->parent); // Create the kobject and add it to the sys/kernel
    if(!BBLKM_kobj){
        printk(KERN_ALERT "BBLKM: cannot create kobject\n");
        return -ENOMEM;
    }

    // create the directory and files in the /sys
    result = sysfs_create_group(BBLKM_kobj, &attr_group);
    if(result) {
        printk(KERN_ALERT "BBLKM: cannot create sys/BBLKM directory\n");
        kobject_put(BBLKM_kobj);                // remove the directory
        return result;
    }
    //took from the example
    getnstimeofday(&ts_last);                          // set the last time to be the current time
    ts_diff = timespec_sub(ts_last, ts_last);          // set the initial time difference to be 0


    ledOn1 = true;
    gpio_request(gpioLED1, "sysfs");          // request the gpio in the sysfile
    gpio_direction_output(gpioLED1, ledOn1);   // Set the gpio to be in output mode and turn off
    gpio_export(gpioLED1, false);               // create a directory of gpio in sys/class/gpio

    ledOn2 = true;
    gpio_request(gpioLED2, "sysfs");          // request the gpio in the sysfile
    gpio_direction_output(gpioLED2, ledOn2);   // Set the gpio to be in output mode and turn off
    gpio_export(gpioLED2, false);           // create a directory of gpio in sys/class/gpio

    ledOn3 = true;
    gpio_request(gpioLED3, "sysfs");          // request the gpio in the sysfile
    gpio_direction_output(gpioLED3, ledOn3);   // Set the gpio to be in output mode and turn off
    gpio_export(gpioLED3, false);               // create a directory of gpio in sys/class/gpio


    gpio_request(gpioButton, "sysfs");       // Set up the gpioButton
    gpio_direction_input(gpioButton);        // Set the button GPIO to be an input
    gpio_set_debounce(gpioButton, DEBOUNCE_TIME); // Debounce the button with a delay
    gpio_export(gpioButton, false);          // create a directory of gpio in sys/class/gpio


    irqNumber = gpio_to_irq(gpioButton);

    //Took from the example
    if(!isRising){                           // If the kernel parameter isRising=0 is supplied
        IRQflags = IRQF_TRIGGER_FALLING;      // Set the interrupt to be on the falling edge
    }
    //Took from the example
    result = request_irq(irqNumber,             // The interrupt number requested
                         (irq_handler_t) BBLKM_irq_handler, // The pointer to the handler function below
                         IRQflags,              // Use the custom kernel param to set interrupt type
                         "BBLKM_handler",  // Used in /proc/interrupts to identify the owner
                         NULL);                 // The *dev_id for shared interrupt lines, NULL is okay


    task = kthread_run(ledControl, NULL, "LED_burst_thread");  // Start the LEDs burst thread
    if(IS_ERR(task)){
        printk(KERN_ALERT "EBB LED: failed to create the task\n");
        return PTR_ERR(task);
    }

    return result;
}



//function to clean the directorieszs
static void __exit BBLKM_exit(void){
    kthread_stop(task);                      // Stop the thread
    kobject_put(BBLKM_kobj);                 //Delete the directories
    free_irq(irqNumber, NULL);// Free the IRQ number, no *dev_id required in this case
    //this instructions unexport the gpios and set value to 0
    gpio_set_value(gpioLED1, 0);
    gpio_unexport(gpioLED1);
    gpio_free(gpioLED1);

    gpio_set_value(gpioLED2, 0);
    gpio_unexport(gpioLED2);
    gpio_free(gpioLED2);

    gpio_set_value(gpioLED3, 0);
    gpio_unexport(gpioLED3);
    gpio_free(gpioLED3);

    gpio_unexport(gpioButton);
    gpio_free(gpioButton);
    printk(KERN_INFO "BBLKM: End of the LKM\n");
}

static irq_handler_t BBLKM_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
    printk(KERN_INFO "BBLKM: Status of leds changed\n");
    getnstimeofday(&ts_current);         // get the time
    ts_diff = timespec_sub(ts_current, ts_last);   // determine the diference between presses
    ts_last = ts_current;
    whiFlag = !whiFlag;
    LEDStatus = !LEDStatus;
    buttonStats++;                     // increment the number presses
    return (irq_handler_t) IRQ_HANDLED;

}


module_init(BBLKM_init);
module_exit(BBLKM_exit);
