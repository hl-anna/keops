
__global__ void GpuConv1DOnDevice(int nx, int ny, float *out, float **args) {

  // get the index of the current thread
  int i = blockIdx.x * blockDim.x + threadIdx.x;

  // declare shared mem
  extern __shared__ float yj[];

  // load parameter(s)
  float param_loc[0 < 1 ? 1 : 0];
   //load<DIMSP, INDSP>(0, param_loc, args); // load parameters variables from global memory to local thread memory

  float fout[3];
  // get the value of variable (index with i)
  float xi[3 < 1 ? 1 : 3];
  float acc[3];
  
  if (i < nx) {
    #pragma unroll
for(int k=0; k<3; k++)
    acc[k] = (float)(0.0f); //<__TYPEACC__, TYPE >()(acc); // acc = 0
    xi[0] = args[0][i*3+0];
xi[1] = args[0][i*3+1];
xi[2] = args[0][i*3+2];
 //<DIMSX, INDSI>(i, xi, args); // load xi variables from global memory to local thread memory
  }

  for (int jstart = 0, tile = 0; jstart < ny; jstart += blockDim.x, tile++) {

    // get the current column
    int j = tile * blockDim.x + threadIdx.x;

    if (j < ny) { // we load yj from device global memory only if j<ny
      (yj + threadIdx.x * 3)[0] = args[1][j*3+0];
(yj + threadIdx.x * 3)[1] = args[1][j*3+1];
(yj + threadIdx.x * 3)[2] = args[1][j*3+2];
 //<DIMSY,INDSJ>(j, yj + threadIdx.x * DIMY, args); // load yj variables from global memory to shared memory
    }
    __syncthreads();

    if (i < nx) { // we compute x1i only if needed
      float * yjrel = yj; // Loop on the columns of the current block.
      for (int jrel = 0; (jrel < blockDim.x) && (jrel < ny - jstart); jrel++, yjrel += 3) {
        
{
// Starting code block for Var(0,3,0)+Var(1,3,1).

#pragma unroll
for(int k=0; k<3; k++) {
    fout[k*1] = (xi+0)[k*1]+((yj + threadIdx.x * 3)+0)[k*1];
 }


// Finished code block for Var(0,3,0)+Var(1,3,1).
}

 //<DIMSX, DIMSY, DIMSP>(fun,fout,xi,yjrel,param_loc); // Call the function, which outputs results in fout
	    #pragma unroll
for(int k=0; k<3; k++) {
    acc[k*1] += (float)(fout[k*1]); }
 //<__TYPEACC__,TYPE>()(acc, fout, jrel + tile * blockDim.x);     // acc += fout
      }
    }
    __syncthreads();
  }
  if (i < nx) {
    #pragma unroll
for(int k=0; k<3; k++)
    (out + i * 3)[k] = (float)acc[k];
 //<__TYPEACC__,TYPE>()(acc, out + i * DIMOUT, i);
  }

}





  extern "C" __host__ int Eval(int nx, int ny, float *out , float* arg0, float* arg1, float* arg2, float* arg3, float* arg4, float* arg5) {

	float* args[6];
	args[0] = arg0;
args[1] = arg1;
args[2] = arg2;
args[3] = arg3;
args[4] = arg4;
args[5] = arg5;

		  
    // device array of pointers to device data
    float **args_d;

    // single cudaMalloc
    cudaMalloc(&args_d, sizeof(float *) * 2);

    cudaMemcpy(args_d, args, 2 * sizeof(float *), cudaMemcpyHostToDevice);

    // Compute on device : grid and block are both 1d

    //int dev = -1;
    //cudaGetDevice(&dev);

    //SetGpuProps(dev);

    dim3 blockSize;

    blockSize.x = 192;
	
    dim3 gridSize;
    gridSize.x = nx / blockSize.x + (nx % blockSize.x == 0 ? 0 : 1);

    GpuConv1DOnDevice <<< gridSize, blockSize, blockSize.x * 3 * sizeof(float) >>> (nx, ny, out, args_d);
    
    // block until the device has completed
    cudaDeviceSynchronize();

    //CudaCheckError();

    cudaFree(args_d);

    return 0;
  }