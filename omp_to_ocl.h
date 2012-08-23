#ifndef __OMP_TO_OCL_H__
#define __OMP_TO_OCL_H__

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#define MAX_SOURCE_SIZE (0x100000)

cl_context ocl_ctx = NULL;
cl_command_queue cmd_q = NULL;
cl_device_id d_id = NULL;

/* buffer objects todo: make an array to dynm create buffers*/
cl_mem buff1 = NULL;
cl_mem buff2 = NULL;
cl_mem buff3 = NULL;

/* kernel and program objects */
cl_kernel kernel;
cl_program program;

#define CHECK(ret) if (ret != CL_SUCCESS) { printf("!!! OpenCL error\n"); exit(1); }
#define CHECKMSG(ret, str) if (ret != CL_SUCCESS) { printf("!!! OpenCL error: %s\n", str); exit(1); }


void o2o_init()
{
    cl_int ret = 0;
    
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_platforms = 0, ret_num_devices = 0;

    ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    CHECK(ret);

    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &d_id, &ret_num_devices);
    CHECK(ret);

    ocl_ctx = clCreateContext(NULL, 1, &d_id, NULL, NULL, &ret);
    CHECK(ret);
}

void o2o_create_cmd_queue()
{
    cl_int ret = 0;
    cmd_q = clCreateCommandQueue(ocl_ctx, d_id, 0, &ret);
    CHECK(ret);
}

void o2o_create_cmd_queue(cl_command_queue_properties flags)
{
    cl_int ret = 0;
    cmd_q = clCreateCommandQueue(ocl_ctx, d_id, flags, &ret);
    CHECK(ret);
}

void o2o_create_buffers()
{
    cl_int ret = 0;
    buff1 = clCreateBuffer(ocl_ctx, CL_MEM_READ_WRITE, sizeof(int), NULL, &ret);
    buff2 = clCreateBuffer(ocl_ctx, CL_MEM_READ_WRITE, sizeof(int), NULL, &ret);
    buff3 = clCreateBuffer(ocl_ctx, CL_MEM_READ_WRITE, sizeof(int), NULL, &ret);
}

cl_mem o2o_create_buffer(cl_mem_flags flags, size_t buff_size)
{
    cl_int ret = 0;
    return clCreateBuffer(ocl_ctx, flags, buff_size, NULL, &ret);
}

cl_mem o2o_create_buffer(void* arg, size_t buff_size)
{
    cl_int ret = 0;
    return clCreateBuffer(ocl_ctx, CL_MEM_READ_WRITE, buff_size, arg, &ret);
}

cl_mem o2o_create_buffer(cl_mem_flags flags, size_t buff_size, void* arg_val)
{
    cl_int ret = 0;
    return clCreateBuffer(ocl_ctx, flags | CL_MEM_COPY_HOST_PTR, buff_size, arg_val, &ret);
}

void o2o_write_buffer(cl_mem& buffer, size_t buff_size, const void* source)
{
    cl_int ret = 0;
    ret = clEnqueueWriteBuffer(cmd_q, buffer, CL_TRUE, 0, buff_size, source, 0, NULL, NULL);
}

void o2o_read_buffer(cl_mem& buffer, size_t buff_size, void* dest)
{
    cl_int ret = 0;
    ret = clEnqueueReadBuffer(cmd_q, buffer, CL_TRUE, 0, buff_size, dest, 0, NULL, NULL);
}

void o2o_create_program_from_file(const char *file_name)
{
    char *kernel_code;
    size_t source_size;
    cl_int ret = 0;

    // Load kernel source file
    FILE *fp = fopen(file_name, "r");
    
    // change this to not use exit?
    if (!fp) 
    {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    
    kernel_code = (char *)malloc(MAX_SOURCE_SIZE);

    source_size = fread(kernel_code, 1, MAX_SOURCE_SIZE, fp);

    fclose(fp);
    
    program = clCreateProgramWithSource(ocl_ctx, 1, 
                                        (const char **)&kernel_code, 
                                        (const size_t *)&source_size, 
                                        &ret);

    free(kernel_code);
    fclose(fp);
}

void o2o_create_program_from_source(const char* kernel_code)
{
    cl_int ret = 0;

    size_t k_size = strlen(kernel_code);
    program = clCreateProgramWithSource(ocl_ctx, 1, 
                                    &kernel_code, 
                                    &k_size,
                                    &ret);

    CHECK(ret);
}

void o2o_build_program()
{
    cl_int ret = 0;

    ret = clBuildProgram(program, 1, &d_id, NULL, NULL, NULL);

    CHECKMSG(ret, "Error building source");
}

void o2o_create_kernel(const char *kernel_function_name)
{
    cl_int ret = 0;

    kernel = clCreateKernel(program, kernel_function_name, &ret);

    CHECK(ret);
}

/**
* Opens an OpenCL kernel source file, builds it and creates a kernel object
* with the given kernel function name.
*/
void o2o_open_and_build(const char* filename, const char* kernel_func_name)
{
    o2o_create_program_from_file(filename);
    o2o_build_program();
    o2o_create_kernel(kernel_func_name);
}

void o2o_set_kernel_arg(int index, size_t arg_size, cl_mem* arg_value)
{
    cl_int ret = 0;

    //ret = clSetKernelArg(kernel, index, arg_size, &arg_value);
    ret = clSetKernelArg(kernel, index, arg_size, (void*)arg_value);
    //ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buff2);
    //ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buff3);

}

void o2o_execute_kernel(size_t global_size, size_t local_size)
{
    cl_int ret = 0;
    ret = clEnqueueNDRangeKernel(cmd_q, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
}


void o2o_execute_kernel(size_t global_size)
{
    cl_int ret = 0;
    ret = clEnqueueNDRangeKernel(cmd_q, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
}

static int finalized = 0;
void o2o_finalize(cl_mem& buff)
{
    cl_int ret = 0;
    
    if (!finalized)
    {
        ret = clFlush(cmd_q);
        ret = clFinish(cmd_q);

        ret = clReleaseKernel(kernel);
        ret = clReleaseProgram(program);

        ret = clReleaseMemObject(buff);

        ret = clReleaseCommandQueue(cmd_q);
        ret = clReleaseContext(ocl_ctx);
        finalized = 1;
    }
    else
        ret = clReleaseMemObject(buff);

}

void o2o_finalize()
{
    cl_int ret = 0;
    
    {
        ret = clFlush(cmd_q);
        ret = clFinish(cmd_q);

        ret = clReleaseKernel(kernel);
        ret = clReleaseProgram(program);

        //ret = clReleaseMemObject(buff);

        ret = clReleaseCommandQueue(cmd_q);
        ret = clReleaseContext(ocl_ctx);
        finalized = 1;
    }

}


#endif //__OMP_TO_OCL_H__