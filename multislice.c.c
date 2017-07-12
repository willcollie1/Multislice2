/* This program uses the "Multislice Algorithm" to simulate the wavefunction for a TEM electron as it passes through a sample.
The multislice method reduces to a succession of transmission and propagation operations with a fast Fourier transform in 
between each. FFTs are calculated using the FFTW libary. The final wavefunction is outputted to the file called "output.txt" 
which is used by the python script "program.py" to create an image of the specimin */ 


#include <stdio.h>
#include <stdlib.h>
#include <math.h>    
#include <complex.h> /* Makes the use of complex numbers much easier */
#include <fftw3.h>  /* Used for FFTs */
#include "multislice.h" /* Contains definitions and some function declarations */ 


int main()
{

  printf("Lattice Parameter (a) in Angstroms:");
  scanf("%lf",&a);

  printf("Lattice Parameter (b) in Angstroms:");
  scanf("%lf",&b);

  printf("Lattice parameter (c) in Angstroms:");
  scanf("%lf",&c);

  printf("Microscope Voltage (volts):");
  scanf("%lf",&v);

  printf("Number of Voxels in x direction:");
  scanf("%d",&height);

  printf("Number of Voxels in y direction:");
  scanf("%d",&width);

  printf("Number of Voxels in z direction:");
  scanf("%d",&depth);

  /* <a b c> Lattice Parameters in Angstroms */
  a = a * angstrom; /* 2.54 */
  b = b * angstrom; /* 4.36 */
  c = c * angstrom; /* 28.00 */
  

  /* Size of an individual "voxel"  */
  double dx = (a / height);
  double dy = (b / width);
  double dz = (c / depth);  /* Slicethickness */ 
  int size = height*width*depth; /* The number of potential readings in file */
  int wavefunctionsize = height*width; /* Number of points wavefunction is evaluated at */
  
      
  /* Output the wavefunction parameters to the file "size.text" (used in python script) */ 
  outputparameters();

  
  /*Allocating memory to store data from potential file in three seperate arrays */ 
  double *x, *y, *V; 
  x  = malloc((size)*sizeof(double));
  y  = malloc((size)*sizeof(double));
  V  = malloc((size)*sizeof(double));

 
  /* Reading in the potential file "potential.txt" */ 
    FILE *potential = fopen("potential.txt", "r");
  for (i=0; i<size; i++){
    fscanf(potential, "%lf %lf %*f  %lf", &x[i], &y[i], &V[i]);
  }
  fclose(potential);
  
  /* Converting potentials from Hartrees to Volts and positions into metres */ 
  for (i=0; i<size; i++){
    V[i] = V[i] * hartree/charge;  
    x[i] = x[i] * dx;
    y[i] = y[i] * dy;
  }
  

  /* Relativistic de broglie wavelength and Interaction parameter (sigma) */ 
  double wavelength = calcwavelength(v);
  double sigma = calcsigma(wavelength); 

  
  /* Transmission Function */
  double complex *T; 
  T  = malloc((size)*sizeof(double complex));
  for (i=0; i<size; i++){
    T[i] = cexp(I*sigma*V[i]*dz);  
  }
  

  /* Propogation Function in k space */
  double complex *P; 
  P  = malloc((size)*sizeof(double complex));
  for (i=0; i<size; i++){
    P[i] = cexp(-I*pi*dz*wavelength*(pow((y[i])/(dy*b),2)+pow((x[i])/(dx*a),2)));
  }


  /* Initialising and assigning memory for wavefunction and wavefunction_next */
  double complex *wavefunction, *wavefunction_next; 
  wavefunction = malloc((wavefunctionsize)*sizeof(double complex));
  wavefunction_next = malloc((wavefunctionsize)*sizeof(double complex));
  
 
    /* Set initial wavefunction equal to 1 */
  for (i=0; i<wavefunctionsize; i++){
    wavefunction[i] = 1;
  }
  
  
  
  /* Initialising and allocating memory for FFT function */
  fftw_complex *in, *out;
  fftw_plan plan;
  fftw_plan planinverse;
  int n[2];
  n[0] = height;
  n[1] = width;
  in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * wavefunctionsize);
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * wavefunctionsize);
  plan = fftw_plan_dft_2d(width, height, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  planinverse = fftw_plan_dft_2d(width, height, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
  
   
 /************************************************** Multislice Iteration loop **********************************************************/
  
  /* Looping over the number of slices */ 
  for(j=0; j<depth; j++){
    
    /* Multiplying the Wavefunction by the Transmission function */ 
    for (i=0; i<wavefunctionsize; i++){
      wavefunction[i] = T[i+(wavefunctionsize*j)]*wavefunction[i];
    }

    /* Taking the 2D Fourier transform of this product */ 
    for (i=0; i<wavefunctionsize; i++){
      in[i] = wavefunction[i];
    }
    fftw_execute(plan);

    
    /* Multiplying by the Propogation function and taking the Inverse FT  */ 
    for (i=0; i<wavefunctionsize; i++){
      wavefunction[i] = out[i]*P[i+(wavefunctionsize*j)];
    }
    
    fftw_execute(planinverse);
  
    for (i=0;i<wavefunctionsize;i++){
      wavefunction_next[i] = out[i] / pow(wavefunctionsize,0.5) ; /* Scaling factor */
    }

    swap_mem(&wavefunction_next,&wavefunction);  /* Swapping memory for each iteration */
  }
  
  /**********************************************End of Multislice Loop **********************************************************/
  
    /* Fourier Transform of Exit Wavefunction */ 
    for (i=0; i<wavefunctionsize; i++){
      in[i] = wavefunction[i];
    }
    
    fftw_execute(plan); 
  
    /* Calculating the point spread function of the objective lens */
    double complex *susceptability, *PSF;
    susceptability = malloc((wavefunctionsize)*sizeof(double complex));
    PSF = malloc((wavefunctionsize)*sizeof(double complex));
    for (i=0; i<wavefunctionsize; i++) {
      susceptability[i] = (2*pi/wavelength)*((0.25*abberation*pow(wavelength,4)*((pow((y[i])/(dy*b),4)+pow(((x[i])/(dx*a)),4))))-(0.5*df*pow(wavelength,2)*((pow((y[i])/(dy*b),2)+pow(((x[i])/(dx*a)),2)))));
      PSF[i] = cexp(-I*susceptability[i]);
    }
    
    /* FT of exit wavefunction multiplied by the PSF */ 
    for (i=0; i<wavefunctionsize; i++) {
      wavefunction[i] = out[i]*PSF[i];
    }
    
    /* Inverse FT and calculate square modulus to get final image intensity */
    for (i=0; i<wavefunctionsize; i++){
      in[i] = wavefunction[i];
    }
    
    fftw_execute(planinverse);

    for (i=0; i<wavefunctionsize; i++){
      wavefunction[i] = out[i] / pow(wavefunctionsize,0.5);  /* Scaling factor */ 
    }
  
    
  /* Output the Intensity at each point to a file called "output.txt" */ 
  FILE* output;
  output = fopen("output.txt", "w");
  for (i=0; i<wavefunctionsize; i++){   
    fprintf(output, "%.20lf \n", cabs(wavefunction[i]));
  }
  fclose(output);
   


   /* Free the dynamic arrays */
   fftw_destroy_plan(plan);
   fftw_destroy_plan(planinverse);
   fftw_free(in); fftw_free(out); 
   free(T), free(P), free(wavefunction), free(wavefunction_next), free(x), free(y), free(V);

}