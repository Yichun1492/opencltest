/*
http://www.kimicat.com/opencl-1/opencl-jiao-xue-yi#TOC-OpenCL-3
在 OpenCL 中，則大致的流程是：
把 OpenCL 裝置初始化。
在 OpenCL 裝置上配置三塊記憶體，以存放 a、b、c 三個陣列的資料。
把 a 陣列和 b 陣列的內容，複製到 OpenCL 裝置上。
編譯要執行的 OpenCL 程式（稱為 kernel）。
執行編譯好的 kernel。
把計算結果從 OpenCL 裝置上，複製到 result 陣列中。
*/

#include <iostream>
#include "CL/cl.h"
#include <fstream>
#include <vector>
#include <chrono>

//要編譯kernel 程式，首先要把檔案內容讀進來，再使用 clCreateProgramWithSource 這個函式，然後再使用 clBuildProgram 編譯。如下所示
cl_program load_program(cl_context context, const char* filename);

int main(void) {
    std::chrono::steady_clock::time_point beg_time = std::chrono::steady_clock::now();
    //先取得 platform 的數目
    cl_int err;
    cl_uint num;
    err = clGetPlatformIDs(0,0, &num);
    if(err != CL_SUCCESS)
    {
        std::cerr<<"Unable to get platforms\n";
        return 0;
    }

    /*
    再取得 platform 的 ID，這在建立 OpenCL context 時會用到
    在 OpenCL 中，類似這樣的模式很常出現：先呼叫第一次以取得數目，以便配置足夠的記憶體量。接著，再呼叫第二次，取得實際的資料。
    */
    std::vector<cl_platform_id> platforms(num);
    err = clGetPlatformIDs(num, &platforms[0], &num);
    if(err != CL_SUCCESS)
    {
        std::cerr<<"Unable to get Platform ID\n";
        return 0;
    }

    /*
    建立一個 OpenCL context
    clCreateContextFromType 是一個 OpenCL 的 API，它可以從指定的裝置類別中，建立一個 OpenCL context。第一個參數是指定 context 的 property。在 OpenCL 中，是透過一個 property 的陣列，以「property 種類」及「property 內容」成對出現，並以 0 做為結束。例如，以上面的例子來說，要指定的 property 種類是 CL_CONTEXT_PLATFORM，即要使用的 platform ID，而 property 內容則是由之前取得的 platform ID 中的第一個（即 platforms[0]）。由於 property 的內容可能是不同的資料型態，因此需要使用 reinterpret_cast 來進行強制轉型。
    第二個參數可以指定要使用的裝置類別。目前可以使用的類別包括：
        CL_DEVICE_TYPE_CPU：使用 CPU 裝置
        CL_DEVICE_TYPE_GPU：使用顯示晶片裝置
        CL_DEVICE_TYPE_ACCELERATOR：特定的 OpenCL 加速裝置，例如 CELL
        CL_DEVICE_TYPE_DEFAULT：系統預設的 OpenCL 裝置
        CL_DEVICE_TYPE_ALL：所有系統中的 OpenCL 裝置
    */
    cl_context_properties prop[] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platforms[0]), 0 };
    cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_DEFAULT, NULL, NULL, NULL);
    if(context == 0) 
    {
        std::cerr << "Can't create OpenCL context\n";
        return 0;
    }

    /*
    一個 OpenCL context 中可以包括一個或多個裝置，所以接下來的工作是要取得裝置的列表。要取得任何和 OpenCL context 相關的資料，可以使用 clGetContextInfo 函式。以下是取得裝置列表的方式：
    CL_CONTEXT_DEVICES 表示要取得裝置的列表。和前面取得 platform ID 的情形相同，clGetContextInfo 被呼叫了兩次：第一次是要取得需要存放裝置列表所需的記憶體空間大小（也就是傳入 &cb），然後第二次呼叫才真正取得所有裝置的列表。
    */
    size_t cb;
    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    std::vector<cl_device_id> devices(cb / sizeof(cl_device_id));
    clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, &devices[0], 0);

    /*
    接下來，可能會想要確定倒底找到的 OpenCL 裝置是什麼。所以，可以透過 OpenCL API 取得裝置的名稱，並將它印出來。取得和裝置相關的資料，是使用 clGetDeviceInfo 函式，和前面的 clGetContextInfo 函式相當類似
    */
    clGetDeviceInfo(devices[0], CL_DEVICE_NAME, 0, NULL, &cb);
    std::string devname;
    devname.resize(cb);
    clGetDeviceInfo(devices[0], CL_DEVICE_NAME, cb, &devname[0], 0);
    std::cout << "Device: " << devname.c_str() << "\n"; //Device: QUALCOMM Adreno(TM)

    /*
    建立command queue
    大部份 OpenCL 的操作，都要透過 command queue。Command queue 可以接收對一個 OpenCL 裝置的各種操作，並按照順序執行（OpenCL 也容許把一個 command queue 指定成不照順序執行，即 out-of-order execution
    */
    cl_command_queue queue = clCreateCommandQueue(context, devices[0], 0, 0);
    //把裝置列表中的第一個裝置（即 devices[0]）建立 command queue。如果想要同時使用多個 OpenCL 裝置，則每個裝置都要有自己的 command queue。
    if(queue == 0) {
        std::cerr << "Can't create command queue\n";
        clReleaseContext(context);
        return 0;
    }
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    unsigned long time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-beg_time).count();
    std::cout<<"opencl initial time cost : " <<time_cost <<std::endl;
    //產生一些「測試資料」
    const int DATA_SIZE = 1048576;
    std::vector<float> a(DATA_SIZE), b(DATA_SIZE), res(DATA_SIZE);
    for(int i = 0; i < DATA_SIZE; i++) {
        a[i] = std::rand();
        b[i] = std::rand();
    }
    
    /*
    配置記憶體並複製資料
    要使用 OpenCL 裝置進行運算時，通常會需要在 OpenCL 裝置上配置記憶體，並把資料從主記憶體中複製到裝置上。有些 OpenCL 裝置可以直接從主記憶體存取資料，但是速度通常會比較慢，因為 OpenCL 裝置（例如顯示卡）通常會有專用的高速記憶體。以下的程式配置三塊記憶體：
    clCreateBuffer 函式可以用來配置記憶體。它的第二個參數可以指定記憶體的使用方式，包括：
        CL_MEM_READ_ONLY：表示 OpenCL kernel 只會對這塊記憶體進行讀取的動作
        CL_MEM_WRITE_ONLY：表示 OpenCL kernel 只會對這塊記憶體進行寫入的動作
        CL_MEM_READ_WRITE：表示 OpenCL kernel 會對這塊記憶體進行讀取和寫入的動作
        CL_MEM_USE_HOST_PTR：表示希望 OpenCL 裝置直接使用指定的主記憶體位址。要注意的是，如果 OpenCL 裝置無法直接存取主記憶體，它可能會將指定的主記憶體位址的資料複製到 OpenCL 裝置上。
    CL_MEM_ALLOC_HOST_PTR：表示希望配置的記憶體是在主記憶體中，而不是在 OpenCL 裝置上。不能和 CL_MEM_USE_HOST_PTR 同時使用。
    CL_MEM_COPY_HOST_PTR：將指定的主記憶體位址的資料，複製到配置好的記憶體中。不能和 CL_MEM_USE_HOST_PTR 同時使用。
    第三個參數是指定要配置的記憶體大小，以 bytes 為單位。在上面的程式中，指定的大小是 sizeof(cl_float) * DATA_SIZE。
    第四個參數是指定主記憶體的位置。因為對 cl_a 和 cl_b 來說，在第二個參數中，指定了 CL_MEM_COPY_HOST_PTR，因此要指定想要複製的資料的位址。cl_res 則不需要指定。
    第五個參數是指定錯誤碼的傳回位址。在這裡並沒有使用到。

    如果 clCreateBuffer 因為某些原因無法配置記憶體（例如 OpenCL 裝置上的記憶體不夠），則會傳回 0。要釋放配置的記憶體，可以使用 clReleaseMemObject 函式。
    */
    beg_time = std::chrono::steady_clock::now();
    cl_mem cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &a[0], NULL);
    cl_mem cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &b[0], NULL);
    cl_mem cl_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
    if(cl_a == 0 || cl_b == 0 || cl_res == 0) {
        std::cerr << "Can't create OpenCL buffer\n";
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 0;
    }
    end_time = std::chrono::steady_clock::now();
    time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-beg_time).count();
    std::cout<<"memory copy time cost: " << time_cost << std::endl; 

    //
    beg_time = std::chrono::steady_clock::now();
    cl_program program = load_program(context, "shader.cl");
    if(program == 0) {
        std::cerr << "Can't load or build program\n";
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 0;
    }
    
    /*
    一個 OpenCL kernel 程式裡面可以有很多個函式。因此，還要取得程式中函式的進入點：
    */
    cl_kernel adder = clCreateKernel(program, "adder", 0);
    if(adder == 0) {
        std::cerr << "Can't load kernel\n";
        clReleaseProgram(program);
        clReleaseMemObject(cl_a);
        clReleaseMemObject(cl_b);
        clReleaseMemObject(cl_res);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 0;
    }
    

    /*
    執行opencl kernel
    要執行 kernel 程式，只需要先設定好函式的參數。adder 函式有三個參數要設定：
    設定參數是使用 clSetKernelArg 函式。它的參數很簡單：第一個參數是要設定的 kernel object，第二個是參數的編號（從 0 開始），第三個參數是要設定的參數的大小，第四個參數則是實際上要設定的參數內部。以這裡的 adder 函式來說，三個參數都是指向 memory object 的指標。
    */
    clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
    clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_b);
    clSetKernelArg(adder, 2, sizeof(cl_mem), &cl_res);
    end_time = std::chrono::steady_clock::now();
    time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-beg_time).count();
    std::cout<<"load program time cost: " << time_cost << std::endl; 

    //設定好參數後，就可以開始執行了
    beg_time = std::chrono::steady_clock::now();
    size_t work_size = DATA_SIZE;
    //clEnqueueNDRangeKernel 會把執行一個 kernel 的動作加到 command queue 裡面。第三個參數（1）是指定 work item 數目的維度，在這裡就是一維。第五個參數是指定 work item 的總數目，也就是 DATA_SIZE。後面的參數現在暫時先不用管。
    err = clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, 0);
    //由於執行的結果是在 OpenCL 裝置的記憶體中，所以要取得結果，需要把它的內容複製到 CPU 能存取的主記憶體中
    /*
    clEnqueueReadBuffer 函式會把「將記憶體資料從 OpenCL 裝置複製到主記憶體」的動作加到 command queue 中。第三個參數表示是否要等待複製的動作完成才傳回，CL_TRUE 表示要等待。第五個參數是要複製的資料大小，第六個參數則是目標的位址。
    由於這裡指定要等待複製動作完成，所以當函式傳回時，資料已經完全複製完成了。最後是進行驗證，確定資料正確：
    */
    if(err == CL_SUCCESS) {
        err = clEnqueueReadBuffer(queue, cl_res, CL_TRUE, 0, sizeof(float) * DATA_SIZE, &res[0], 0, 0, 0);
    }
    end_time = std::chrono::steady_clock::now();
    time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-beg_time).count();
    std::cout<<"execute and get result time cost: " << time_cost << std::endl; 
    
    
    //驗證正確性
    beg_time = std::chrono::steady_clock::now();
    if(err == CL_SUCCESS) {
        bool correct = true;
        for(int i = 0; i < DATA_SIZE; i++) {
            if(a[i] + b[i] != res[i]) {
            correct = false;
            break;
            }
        }

        if(correct) {
            std::cout << "Data is correct\n";
        }
        else {
            std::cout << "Data is incorrect\n";
        }
    }
    else {
        std::cerr << "Can't run kernel or read back data\n";
    }
    end_time = std::chrono::steady_clock::now();
    time_cost = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-beg_time).count();
    std::cout<<"cpu time cost: " << time_cost << std::endl; 

    clReleaseKernel(adder);
    clReleaseProgram(program);
    clReleaseMemObject(cl_a);
    clReleaseMemObject(cl_b);
    clReleaseMemObject(cl_res);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    

    // std::ifstream data_file;
    // data_file.open("./single/sunrgbd_val000003_321X241_0.000000.txt", std::ios::in | std::ios::binary);
    // std::vector<float>descriptorsValues;
    // const size_t outputtensor_size = 1701942 ;
    // descriptorsValues.resize(outputtensor_size);
    // data_file.read(reinterpret_cast<char*>(&descriptorsValues[0]), outputtensor_size*sizeof(float));
    // data_file.close();

    // const int m_chanelNum = 22;
    // const int m_modelOutH = 241;
    // const int m_modelOutW = 321;
    // const int m_fitModelOutputSizeH = 233;
    // const int m_fitModelOutputSizeW = 321;

    // for(int row =0 ; row <1 ; row++){
    //     for(int col=0; col<5; col++)
    //     {
    //         if((row<m_fitModelOutputSizeH) && (col< m_fitModelOutputSizeW))
    //         {
    //             std::vector<float> pixel_values(descriptorsValues.begin()+(row*m_modelOutW+col)*m_chanelNum, descriptorsValues.begin()+(row*m_modelOutW+col+1)*m_chanelNum);
    //             float max_prob = 0.0;
    //             int max_index = -1;
    //             for(int channel = 0 ; channel<m_chanelNum;channel++)
    //             {
    //                 if(pixel_values[channel]>max_prob)
    //                 {
    //                     max_prob = pixel_values[channel];
    //                     max_index = channel;
    //                 }
    //             }
    //             //std::cout<<"max_prob : " << max_prob << std::endl;
    //             //std::cout<<"max_index: " << max_index<< std::endl;
    //         }
    //     }
    // }


    // std::chrono::steady_clock::time_point beg_time = std::chrono::steady_clock::now();
    // float a[10000];
    // float b[10000];
    // float c[10000];
    // for(int i =0; i< 10000;i++){
    //     c[i] = a[i]+b[i];
    // }
    // std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    // unsigned long time_cost = std::chrono::duration_cast<std::chrono::microseconds> (end_time-beg_time).count();
    // std::cout << "Add function time : " << time_cost <<std::endl;

    

    return 0;
}

/*
要編譯kernel 程式，首先要把檔案內容讀進來，再使用 clCreateProgramWithSource 這個函式，然後再使用 clBuildProgram 編譯。如下所示
直接將檔案讀到記憶體中，再呼叫 clCreateProgramWithSource 建立一個 program object。建立成功後，再呼叫 clBuildProgram 函式編譯程式。clBuildProgram 函式可以指定很多參數，不過在這裡暫時沒有使用到。
有了這個函式，在 main 函式中，直接呼叫：
*/
cl_program load_program(cl_context context, const char* filename)
{
    std::ifstream in(filename, std::ios_base::binary);
    if(!in.good()) {
        return 0;
    }

    // get file length
    in.seekg(0, std::ios_base::end);
    size_t length = in.tellg();
    in.seekg(0, std::ios_base::beg);

    // read program source
    std::vector<char> data(length + 1);
    in.read(&data[0], length);
    data[length] = 0;

    // create and build program 
    const char* source = &data[0];
    cl_program program = clCreateProgramWithSource(context, 1, &source, 0, 0);
    if(program == 0) {
    return 0;
    }

    if(clBuildProgram(program, 0, 0, 0, 0, 0) != CL_SUCCESS) {
    return 0;
    }

    return program;
}

/*
DATA_SIZE = 10485760
Device: QUALCOMM Adreno(TM)
opencl initial time cost : 22
memory copy time cost: 46
load program time cost: 41
execute and get result time cost: 19
Data is correct
cpu time cost: 90

DATA_SIZE = 1048576
Device: QUALCOMM Adreno(TM)
opencl initial time cost : 22
memory copy time cost: 4
load program time cost: 40
execute and get result time cost: 3
Data is correct
cpu time cost: 8

*/