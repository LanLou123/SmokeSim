
# define OMParallelize

# ifdef OMParallelize
# define TOTALThreads 8
# endif
#include "mac_grid.h"
#include "open_gl_headers.h"
#include "camera.h"
#include "custom_output.h" 
#include "constants.h"
#include <math.h>
#include <map>
#include <stdio.h>
#include <cstdlib>
#include <Eigen/Eigen>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <omp.h>
#undef max
#undef min 
#include <fstream> 

using namespace Eigen;
using namespace std;


// Globals
MACGrid target;


// NOTE: x -> cols, z -> rows, y -> stacks
MACGrid::RenderMode MACGrid::theRenderMode = SHEETS;
bool MACGrid::theDisplayVel = false;//true

#define FOR_EACH_CELL \
   for(int k = 0; k < theDim[MACGrid::Z]; k++)  \
      for(int j = 0; j < theDim[MACGrid::Y]; j++) \
         for(int i = 0; i < theDim[MACGrid::X]; i++) 

#define FOR_EACH_CELL_REVERSE \
   for(int k = theDim[MACGrid::Z] - 1; k >= 0; k--)  \
      for(int j = theDim[MACGrid::Y] - 1; j >= 0; j--) \
         for(int i = theDim[MACGrid::X] - 1; i >= 0; i--) 

#define FOR_EACH_FACE \
   for(int k = 0; k < theDim[MACGrid::Z]+1; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]+1; j++) \
         for(int i = 0; i < theDim[MACGrid::X]+1; i++) 

#define FOR_EACH_FACE_X \
   for(int k = 0; k < theDim[MACGrid::Z]; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]; j++) \
         for(int i = 0; i < theDim[MACGrid::X]+1; i++)

#define FOR_EACH_FACE_Y \
   for(int k = 0; k < theDim[MACGrid::Z]; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]+1; j++) \
         for(int i = 0; i < theDim[MACGrid::X]; i++)

#define FOR_EACH_FACE_Z \
   for(int k = 0; k < theDim[MACGrid::Z]+1; k++) \
      for(int j = 0; j < theDim[MACGrid::Y]; j++) \
         for(int i = 0; i < theDim[MACGrid::X]; i++)

MACGrid::MACGrid()
{
   initialize();
}

MACGrid::MACGrid(const MACGrid& orig)
{
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;
}

MACGrid& MACGrid::operator=(const MACGrid& orig)
{
   if (&orig == this)
   {
      return *this;
   }
   mU = orig.mU;
   mV = orig.mV;
   mW = orig.mW;
   mP = orig.mP;
   mD = orig.mD;
   mT = orig.mT;   

   return *this;
}

MACGrid::~MACGrid()
{
}

void MACGrid::reset()
{
    cout<<"once"<<endl;
   mU.initialize();
   mV.initialize();
   mW.initialize();
   mP.initialize();
   mD.initialize();
   mT.initialize(0.0);

    centeral_vel_x.initialize(0.0);
    centeral_vel_y.initialize(0.0);
    centeral_vel_z.initialize(0.0);

    omegaX.initialize(0.0);
    omegaY.initialize(0.0);
    omegaZ.initialize(0.0);
    omegaN.initialize(0.0);

    omegaGX.initialize(0.0);
    omegaGY.initialize(0.0);
    omegaGZ.initialize(0.0);

    vorConfFX.initialize(0.0);
    vorConfFY.initialize(0.0);
    vorConfFZ.initialize(0.0);

    diverGence.initialize(0.0);

    calculateAMatrix();
    calculatePreconditioner(AMatrix);
    int l=0;
}

void MACGrid::initialize()
{
   reset();

}

void MACGrid::updateSources()
{
    // Set initial values for density, temperature, velocity


//    FOR_EACH_FACE
//            {
//                mU(i,j,k)=1e-3;
//                mV(i,j,k)=1e-3;
//                mW(i,j,k)=1e-3;
//            };
    for(int i=24; i<25;i++){
        for(int j=0; j<3; j++){
            mV(i,j,0) = 30.0;
            mU(i,j,0) = 0.0;
            mD(i,j,0) = 1.0;
            mT(i,j,0) = 1.0;
        }
    }


	// Refresh particles in source.
	for(int i=23; i<24; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k <= 0; k++) {
				vec3 cell_center(theCellSize*(i+0.5), theCellSize*(j+0.5), theCellSize*(k+0.5));
				for(int p=0; p<10; p++) {
                    double a = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                    double b = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                    double c = ((float) rand() / RAND_MAX - 0.5) * theCellSize;
                    vec3 shift(a, b, c);
                    vec3 xp = cell_center + shift;
                    rendering_particles.push_back(xp);
                }
			}
		}
	}
	

}


void MACGrid::advectVelocity(double dt)
{
    // TODO: Calculate new velocities and store in target

//    FOR_EACH_FACE
//            {
//                vec3 currentptfx=vec3(i*theCellSize,j*theCellSize+0.5,k*theCellSize+0.5);
//                vec3 currentptfy=vec3(i*theCellSize+0.5,j*theCellSize,k*theCellSize+0.5);
//                vec3 currentptfz=vec3(i*theCellSize+0.5,j*theCellSize+0.5,k*theCellSize);
////                vec3 cur_vel_x=vec3(mU(i,j,k),0,0);
////                vec3 cur_vel_y=vec3(0,mV(i,j,k),0);
////                vec3 cur_vel_z=vec3(0,0,mW(i,j,k));
//                vec3 cur_vel_x=vec3(getVelocityX(currentptfx),0,0);
//                vec3 cur_vel_y=vec3(0,getVelocityY(currentptfy),0);
//                vec3 cur_vel_z=vec3(0,0,getVelocityZ(currentptfz));
//                vec3 mid_pt_x=currentptfx-cur_vel_x*dt/2;
//                vec3 mid_pt_y=currentptfy-cur_vel_y*dt/2;
//                vec3 mid_pt_z=currentptfz-cur_vel_z*dt/2;
//                vec3 old_pt_x=currentptfx-vec3(getVelocityX(mid_pt_x),0,0)*dt;
//                vec3 old_pt_y=currentptfy-vec3(0,getVelocityY(mid_pt_y),0)*dt;
//                vec3 old_pt_z=currentptfz-vec3(0,0,getVelocityZ(mid_pt_z))*dt;
//                double newvel_x=getVelocityX(old_pt_x);
//                double newvel_y=getVelocityY(old_pt_y);
//                double newvel_z=getVelocityZ(old_pt_z);
//                target.mU(i,j,k)=newvel_x;
//                target.mV(i,j,k)=newvel_y;
//                target.mW(i,j,k)=newvel_z;
////                if(mV(i,j,k)>1)
////                std::cout<<"i:"<<i<<"j:"<<j<<"k:"<<k<<"     "<<mV(i,j,k)<<std::endl;
//            };


            FOR_EACH_FACE_X
                        
                    {
                         vec3 curPos = vec3(i*theCellSize, (j+0.5)*theCellSize, (k+0.5)*theCellSize);
                         vec3 midPos=curPos-getVelocity(curPos)*dt/2;
                         target.mU(i,j,k) = mU.interpolate(curPos - getVelocity(midPos)*dt);
                    };

            FOR_EACH_FACE_Y
                    {
                         vec3 curPos = vec3((i+0.5)*theCellSize, j*theCellSize, (k+0.5)*theCellSize);
                        vec3 midPos=curPos-getVelocity(curPos)*dt/2;
                         target.mV(i,j,k) = mV.interpolate(curPos - getVelocity(midPos)*dt);
                    };
        
    

            FOR_EACH_FACE_Z
                    {
                         vec3 curPos = vec3((i+0.5)*theCellSize, (j+0.5)*theCellSize, k*theCellSize);
                        vec3 midPos=curPos-getVelocity(curPos)*dt/2;
                         target.mW(i,j,k) = mW.interpolate(curPos - getVelocity(midPos)*dt);
                    }
        
 
    // Then save the result to our object.
    mU = target.mU;
    mV = target.mV;
    mW = target.mW;
    // TODO: Get rid of these three lines after you implement yours

    // TODO: Your code is here. It builds target.mU, target.mV and target.mW for all faces
    //
    //
    //
//    FOR_EACH_FACE
//            {
//                if(mV(i,j,k)>1)
//                    std::cout<<mV(i,j,k)<<":"<<i<<","<<j<<","<<k<<std::endl;
//            };
    // Then save the result to our object


}

void MACGrid::advectTemperature(double dt)
{

    FOR_EACH_CELL
            {
                vec3 currentptc=vec3(i*theCellSize+0.5,j*theCellSize+0.5,k*theCellSize+0.5);
                vec3 cur_vel=getVelocity(currentptc);
                vec3 mid_pt=currentptc-cur_vel*dt/2;
                vec3 old_pt=currentptc-getVelocity(mid_pt);
                double new_temp=getTemperature(old_pt);
                target.mT(i,j,k)=new_temp;
            };
    mT =target.mT;
}


void MACGrid::advectRenderingParticles(double dt) {

	rendering_particles_vel.resize(rendering_particles.size());
	for (size_t p = 0; p < rendering_particles.size(); p++) {
		vec3 currentPosition = rendering_particles[p];
        vec3 currentVelocity = getVelocity(currentPosition);
        vec3 nextPosition = currentPosition + currentVelocity * dt;
        vec3 clippedNextPosition = clipToGrid(nextPosition, currentPosition);
        // Keep going...
        vec3 nexv=getVelocity(nextPosition);
        vec3 nextVelocity = getVelocity(clippedNextPosition);
        vec3 averageVelocity = (currentVelocity + nextVelocity) / 2.0;
        vec3 betterNextPosition = currentPosition + averageVelocity * dt;
        vec3 clippedBetterNextPosition = clipToGrid(betterNextPosition, currentPosition);
        rendering_particles[p] = clippedBetterNextPosition;
		rendering_particles_vel[p] = averageVelocity;
//        rendering_particles_vel[p]=nexv;
//        rendering_particles[p]=nextPosition;
	}
}

void MACGrid::advectDensity(double dt)
{

    FOR_EACH_CELL
            {
                vec3 currentptc=vec3(i*theCellSize+0.5,j*theCellSize+0.5,k*theCellSize+0.5);
                double cur_density=getDensity(currentptc);
                vec3 cur_vel=getVelocity(currentptc);
                vec3 mid_pt=currentptc-cur_vel*dt/2;
                vec3 old_pt=currentptc-getVelocity(mid_pt);
                double new_density=getDensity(old_pt);
                target.mD(i,j,k)=new_density;
            };

    mD = target.mD;
}

void MACGrid::computeBouyancy(double dt)
{


    double a = 3.0, b = 3.0;
    FOR_EACH_CELL
            {
                double s = (mD(i,j,k) + mD(i,j-1,k))/2; // CHANGED! Original: double s = (mD(i,j,k) + mD(i,j-1,k))/2;
                double T = (mT(i,j,k)+mT(i,j-1,k))/2;
                double fbuoy = -a*s+b*T;
                target.mV(i,j,k) += dt*fbuoy;
            };

    mV = target.mV;
}

void MACGrid::computeCentralVel() {
    FOR_EACH_CELL
            {
//                centeral_vel_x(i,j,k)=(getVelocityX(vec3(i,j,k))+getVelocityX(vec3(i+1,j,k)))/2;
//                centeral_vel_y(i,j,k)=(getVelocityY(vec3(i,j,k))+getVelocityY(vec3(i,j+1,k)))/2;
//                centeral_vel_z(i,j,k)=(getVelocityZ(vec3(i,j,k))+getVelocityZ(vec3(i,j,k+1)))/2;
                centeral_vel_x(i,j,k)=(mU(i,j,k)+mU(i+1,j,k))/2;
                centeral_vel_y(i,j,k)=(mV(i,j,k)+mV(i,j+1,k))/2;
                centeral_vel_z(i,j,k)=(mW(i,j,k)+mW(i,j,k+1))/2;
            };
}

void MACGrid::computeOmega() {
    FOR_EACH_CELL
            {
                omegaX(i,j,k)=(centeral_vel_z(i,j+1,k)-centeral_vel_z(i,j-1,k)-centeral_vel_y(i,j,k+1)+centeral_vel_y(i,j,k-1))/(2*theCellSize);
                omegaY(i,j,k)=(centeral_vel_x(i,j,k+1)-centeral_vel_x(i,j,k-1)-centeral_vel_z(i+1,j,k)+centeral_vel_z(i-1,j,k))/(2*theCellSize);
                omegaZ(i,j,k)=(centeral_vel_y(i+1,j,k)-centeral_vel_y(i-1,j,k)-centeral_vel_x(i,j+1,k)+centeral_vel_x(i,j-1,k))/(2*theCellSize);
                vec3 normalO=vec3(omegaX(i,j,k),omegaY(i,j,k),omegaZ(i,j,k));
                omegaN(i,j,k)=normalO.Length();

            };
}

void MACGrid::computeOmegaGradient() {
    FOR_EACH_CELL
            {
                omegaGX(i,j,k)=(omegaN(i+1,j,k)-omegaN(i-1,j,k))/(2*theCellSize);
                omegaGY(i,j,k)=(omegaN(i,j+1,k)-omegaN(i,j-1,k))/(2*theCellSize);
                omegaGZ(i,j,k)=(omegaN(i,j,k+1)-omegaN(i,j,k-1))/(2*theCellSize);

            };
}
void MACGrid::computeVorticityConfinement(double dt) {


    // Apply the forces to the current velocity and store the result in target
    // STARTED.
    computeCentralVel();
    computeOmega();
    computeOmegaGradient();

    FOR_EACH_CELL {

                vec3 N = vec3(omegaGX(i, j, k), omegaGY(i, j, k), omegaGZ(i, j, k));
                N = N / (N.Length() + 1e-20);
                vec3 Omega = vec3(omegaX(i, j, k), omegaY(i, j, k), omegaZ(i, j, k));
                vec3 confF = N ^Omega * theCellSize * theVorticityEpsilon;

                vorConfFX(i, j, k) = confF[0];
                vorConfFY(i, j, k) = confF[1];
                vorConfFZ(i, j, k) = confF[2];
            };
    FOR_EACH_FACE_X
            {
                target.mU(i, j, k) += dt * (vorConfFX(i - 1, j, k) + vorConfFX(i, j, k)) / 2;
            };
    FOR_EACH_FACE_Y
            {

                target.mV(i, j, k) += dt * (vorConfFY(i, j - 1, k) + vorConfFY(i, j, k)) / 2;

            };
    FOR_EACH_FACE_Z
            {
                target.mW(i, j, k) += dt * (vorConfFZ(i, j, k - 1) + vorConfFZ(i, j, k)) / 2;
            };

    // Then save the result to our object
    mU = target.mU;
    mV = target.mV;
    mW = target.mW;

}
void MACGrid::addExternalForces(double dt)
{
   computeBouyancy(dt);
   computeVorticityConfinement(dt);
}

void MACGrid::computeDivergence()
{
    FOR_EACH_CELL
            {
                diverGence(i,j,k)=(mU(i+1,j,k)+mV(i,j+1,k)+mW(i,j,k+1)-mU(i,j,k)-mV(i,j,k)-mW(i,j,k))/theCellSize;

            };
}

void MACGrid::project(double dt)
{
    int size=theDim[MACGrid::X]*theDim[MACGrid::Y]*theDim[MACGrid::Z];
    SparseMatrix<double> A(size,size);
    A.setZero();
    VectorXd b(size);
    VectorXd p;
    computeDivergence();

    FOR_EACH_CELL
            {
                int curidx= getCellIndex(i,j,k);

                int numCells=0;
                if(isValidCell(i+1,j,k))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i+1,j,k))=-1;
                }
                if(isValidCell(i-1,j,k))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i-1,j,k))=-1;
                }
                if(isValidCell(i,j+1,k))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i,j+1,k))=-1;
                }
                if(isValidCell(i,j-1,k))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i,j-1,k))=-1;
                }
                if(isValidCell(i,j,k+1))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i,j,k+1))=-1;
                }
                if(isValidCell(i,j,k-1))
                {
                    numCells++;
                    A.insert(curidx,getCellIndex(i,j,k-1))=-1;

                }
//                d(i,j,k)=-pow(theCellSize,2)*diverGence(i,j,k)/dt*theAirDensity;
                b(curidx)=-pow(theCellSize,2)*diverGence(i,j,k)/dt*theAirDensity;
//                cout<<b(curidx)<<endl;
                A.insert(curidx,curidx)=numCells;

            };
//    Eigen::initParallel();
//    Eigen::setNbThreads(5);
//    omp_set_num_threads(5);
//    OMP_NUM_THREADS=n ./my_program
//    omp_set_num_threads(5);
//    typedef ConjugateGradient<SparseMatrix<double>,Lower, IncompleteCholesky<double>> ICCG;

    LeastSquaresConjugateGradient<SparseMatrix<double> > lscg;
//    ICCG cg;
//    cg.setTolerance(0.001);
//    cg.setMaxIterations(500);
    lscg.compute(A);
    VectorXd x = lscg.solve(b);
//    lscg.setMaxIterations(500);
    x=lscg.solve(b);
    std::cout << "#iterations:     " << lscg.iterations() << std::endl;
    std::cout << "estimated error: " << lscg.error()      << std::endl;
    std::cout<<"threads"<<Eigen::nbThreads( )<<endl;



    for(int idx=0;idx<p.size();idx++)
    {
        int u=p.size();
        int i,j,k;
        getCellIndexReverse(idx,i,j,k);
        mP(i,j,k)=p(idx);
    }


    FOR_EACH_FACE_X
        {
            target.mU(i,j,k)+=dt*(mP(i,j,k)-mP(i-1,j,k))/(theCellSize*theAirDensity);
        };
    FOR_EACH_FACE_Y
            {
                target.mV(i,j,k)+=dt*(mP(i,j,k)-mP(i,j-1,k))/(theCellSize*theAirDensity);
            };
    FOR_EACH_FACE_Y
        {
            target.mW(i,j,k)+=dt*(mP(i,j,k)-mP(i,j,k-1))/(theCellSize*theAirDensity);
        };
    mU = target.mU;
    mV = target.mV;
    mW = target.mW;

	#ifdef _DEBUG
	// Check border velocities:
	FOR_EACH_FACE {
		if (isValidFace(MACGrid::X, i, j, k)) {

			if (i == 0) {
				if (abs(target.mU(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW X:  " << target.mU(i,j,k) );
					//target.mU(i,j,k) = 0;
				}
			}

			if (i == theDim[MACGrid::X]) {
				if (abs(target.mU(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH X: " << target.mU(i,j,k) );
					//target.mU(i,j,k) = 0;
				}
			}

		}
		if (isValidFace(MACGrid::Y, i, j, k)) {


			if (j == 0) {
				if (abs(target.mV(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW Y:  " << target.mV(i,j,k) );
					//target.mV(i,j,k) = 0;
				}
			}

			if (j == theDim[MACGrid::Y]) {
				if (abs(target.mV(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH Y: " << target.mV(i,j,k) );
					//target.mV(i,j,k) = 0;
				}
			}

		}
		if (isValidFace(MACGrid::Z, i, j, k)) {

			if (k == 0) {
				if (abs(target.mW(i,j,k)) > 0.0000001) {
					PRINT_LINE( "LOW Z:  " << target.mW(i,j,k) );
					//target.mW(i,j,k) = 0;
				}
			}

			if (k == theDim[MACGrid::Z]) {
				if (abs(target.mW(i,j,k)) > 0.0000001) {
					PRINT_LINE( "HIGH Z: " << target.mW(i,j,k) );
					//target.mW(i,j,k) = 0;
				}
			}
		}
	}
	#endif


   // Then save the result to our object


	#ifdef _DEBUG
   // IMPLEMENT THIS AS A SANITY CHECK: assert (checkDivergence());
   // TODO: Fix duplicate code:
   FOR_EACH_CELL {
	   // Construct the vector of divergences d:
        double velLowX = mU(i,j,k);
        double velHighX = mU(i+1,j,k);
        double velLowY = mV(i,j,k);
        double velHighY = mV(i,j+1,k);
        double velLowZ = mW(i,j,k);
        double velHighZ = mW(i,j,k+1);
		double divergence = ((velHighX - velLowX) + (velHighY - velLowY) + (velHighZ - velLowZ)) / theCellSize;
		if (abs(divergence) > 0.02 ) {
			PRINT_LINE("WARNING: Divergent! ");
			PRINT_LINE("Divergence: " << divergence);
			PRINT_LINE("Cell: " << i << ", " << j << ", " << k);
		}
   }
	#endif


}

vec3 MACGrid::getVelocity(const vec3& pt)
{
   vec3 vel;
   vel[0] = getVelocityX(pt); 
   vel[1] = getVelocityY(pt); 
   vel[2] = getVelocityZ(pt); 
   return vel;
}

double MACGrid::getVelocityX(const vec3& pt)
{
   return mU.interpolate(pt);
}

double MACGrid::getVelocityY(const vec3& pt)
{
   return mV.interpolate(pt);
}

double MACGrid::getVelocityZ(const vec3& pt)
{
   return mW.interpolate(pt);
}

double MACGrid::getTemperature(const vec3& pt)
{
   return mT.interpolate(pt);
}

double MACGrid::getDensity(const vec3& pt)
{
   return mD.interpolate(pt);
}

vec3 MACGrid::getCenter(int i, int j, int k)
{
   double xstart = theCellSize/2.0;
   double ystart = theCellSize/2.0;
   double zstart = theCellSize/2.0;

   double x = xstart + i*theCellSize;
   double y = ystart + j*theCellSize;
   double z = zstart + k*theCellSize;
   return vec3(x, y, z);
}


vec3 MACGrid::getRewoundPosition(const vec3 & currentPosition, const double dt) {

	/*
	// EULER (RK1):
	vec3 currentVelocity = getVelocity(currentPosition);
	vec3 rewoundPosition = currentPosition - currentVelocity * dt;
	vec3 clippedRewoundPosition = clipToGrid(rewoundPosition, currentPosition);
	return clippedRewoundPosition;
	*/

	// HEUN / MODIFIED EULER (RK2):
	vec3 currentVelocity = getVelocity(currentPosition);
	vec3 rewoundPosition = currentPosition - currentVelocity * dt;
	vec3 clippedRewoundPosition = clipToGrid(rewoundPosition, currentPosition);
	// Keep going...
	vec3 rewoundVelocity = getVelocity(clippedRewoundPosition);
	vec3 averageVelocity = (currentVelocity + rewoundVelocity) / 2.0;
	vec3 betterRewoundPosition = currentPosition - averageVelocity * dt;
	vec3 clippedBetterRewoundPosition = clipToGrid(betterRewoundPosition, currentPosition);
	return clippedBetterRewoundPosition;

}


vec3 MACGrid::clipToGrid(const vec3& outsidePoint, const vec3& insidePoint) {
	/*
	// OLD:
	vec3 rewindPosition = outsidePoint;
	if (rewindPosition[0] < 0) rewindPosition[0] = 0; // TEMP!
	if (rewindPosition[1] < 0) rewindPosition[1] = 0; // TEMP!
	if (rewindPosition[2] < 0) rewindPosition[2] = 0; // TEMP!
	if (rewindPosition[0] > theDim[MACGrid::X]) rewindPosition[0] = theDim[MACGrid::X]; // TEMP!
	if (rewindPosition[1] > theDim[MACGrid::Y]) rewindPosition[1] = theDim[MACGrid::Y]; // TEMP!
	if (rewindPosition[2] > theDim[MACGrid::Z]) rewindPosition[2] = theDim[MACGrid::Z]; // TEMP!
	return rewindPosition;
	*/

	vec3 clippedPoint = outsidePoint;

	for (int i = 0; i < 3; i++) {
		if (clippedPoint[i] < 0) {
			vec3 distance = clippedPoint - insidePoint;
			double newDistanceI = 0 - insidePoint[i];
			double ratio = newDistanceI / distance[i];
			clippedPoint = insidePoint + distance * ratio;
		}
		if (clippedPoint[i] > getSize(i)) {
			vec3 distance = clippedPoint - insidePoint;
			double newDistanceI = getSize(i) - insidePoint[i];
			double ratio = newDistanceI / distance[i];
			clippedPoint = insidePoint + distance * ratio;
		}
	}

#ifdef _DEBUG
	// Make sure the point is now in the grid:
	if (clippedPoint[0] < 0 || clippedPoint[1] < 0 || clippedPoint[2] < 0 || clippedPoint[0] > getSize(0) || clippedPoint[1] > getSize(1) || clippedPoint[2] > getSize(2)) {
		PRINT_LINE("WARNING: Clipped point is outside grid!");
	}
#endif


	return clippedPoint;

}


double MACGrid::getSize(int dimension) {
	return theDim[dimension] * theCellSize;
}


int MACGrid::getCellIndex(int i, int j, int k)
{
	return i + j * theDim[MACGrid::X] + k * theDim[MACGrid::Y] * theDim[MACGrid::X];
}
void MACGrid::getCellIndexReverse(int idx, int &i, int &j, int &k) {
    i=idx/theDim[MACGrid::X];
    j=idx/(theDim[MACGrid::X]*theDim[MACGrid::Y]);
    k=idx/(theDim[MACGrid::X]*theDim[MACGrid::Y]*theDim[MACGrid::Z]);
}

int MACGrid::getNumberOfCells()
{
	return theDim[MACGrid::X] * theDim[MACGrid::Y] * theDim[MACGrid::Z];
}


bool MACGrid::isValidCell(int i, int j, int k)
{
	if (i >= theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
		return false;
	}

	if (i < 0 || j < 0 || k < 0) {
		return false;
	}

	return true;
}


bool MACGrid::isValidFace(int dimension, int i, int j, int k)
{
	if (dimension == 0) {
		if (i > theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
			return false;
		}
	} else if (dimension == 1) {
		if (i >= theDim[MACGrid::X] || j > theDim[MACGrid::Y] || k >= theDim[MACGrid::Z]) {
			return false;
		}
	} else if (dimension == 2) {
		if (i >= theDim[MACGrid::X] || j >= theDim[MACGrid::Y] || k > theDim[MACGrid::Z]) {
			return false;
		}
	}

	if (i < 0 || j < 0 || k < 0) {
		return false;
	}

	return true;
}


vec3 MACGrid::getFacePosition(int dimension, int i, int j, int k)
{
	if (dimension == 0) {
		return vec3(i * theCellSize, (j + 0.5) * theCellSize, (k + 0.5) * theCellSize);
	} else if (dimension == 1) {
		return vec3((i + 0.5) * theCellSize, j * theCellSize, (k + 0.5) * theCellSize);
	} else if (dimension == 2) {
		return vec3((i + 0.5) * theCellSize, (j + 0.5) * theCellSize, k * theCellSize);
	}

	return vec3(0,0,0); //???

}

void MACGrid::calculateAMatrix() {

	FOR_EACH_CELL {

		int numFluidNeighbors = 0;
		if (i-1 >= 0) {
			AMatrix.plusI(i-1,j,k) = -1;
			numFluidNeighbors++;
		}
		if (i+1 < theDim[MACGrid::X]) {
			AMatrix.plusI(i,j,k) = -1;
			numFluidNeighbors++;
		}
		if (j-1 >= 0) {
			AMatrix.plusJ(i,j-1,k) = -1;
			numFluidNeighbors++;
		}
		if (j+1 < theDim[MACGrid::Y]) {
			AMatrix.plusJ(i,j,k) = -1;
			numFluidNeighbors++;
		}
		if (k-1 >= 0) {
			AMatrix.plusK(i,j,k-1) = -1;
			numFluidNeighbors++;
		}
		if (k+1 < theDim[MACGrid::Z]) {
			AMatrix.plusK(i,j,k) = -1;
			numFluidNeighbors++;
		}
		// Set the diagonal:
		AMatrix.diag(i,j,k) = numFluidNeighbors;
	}
}

bool MACGrid::preconditionedConjugateGradient(const GridDataMatrix & A, GridData & p, const GridData & d, int maxIterations, double tolerance) {
	// Solves Ap = d for p.

	FOR_EACH_CELL {
		p(i,j,k) = 0.0; // Initial guess p = 0.	
	}

	GridData r = d; // Residual vector.

	/*
	PRINT_LINE("r: ");
	FOR_EACH_CELL {
		PRINT_LINE(r(i,j,k));
	}
	*/
	GridData z; z.initialize();
	applyPreconditioner(r, A, z); // Auxillary vector.
	/*
	PRINT_LINE("z: ");
	FOR_EACH_CELL {
		PRINT_LINE(z(i,j,k));
	}
	*/

	GridData s;
    s.initialize();
    s = z; // Search vector;

	double sigma = dotProduct(z, r);

	for (int iteration = 0; iteration < maxIterations; iteration++) {

		double rho = sigma; // According to TA. Here???

		apply(A, s, z); // z = applyA(s);

		double alpha = rho/dotProduct(z, s);

		GridData alphaTimesS; alphaTimesS.initialize();
		multiply(alpha, s, alphaTimesS);
		add(p, alphaTimesS, p);
		//p += alpha * s;

		GridData alphaTimesZ; alphaTimesZ.initialize();
		multiply(alpha, z, alphaTimesZ);
		subtract(r, alphaTimesZ, r);
		//r -= alpha * z;
        FOR_EACH_CELL{
                    cout<<r(i,j,k)<<endl;
                };
		if (maxMagnitude(r) <= tolerance) {
			//PRINT_LINE("PCG converged in " << (iteration + 1) << " iterations.");
			return true; //return p;
		}

		applyPreconditioner(r, A, z); // z = applyPreconditioner(r);

		double sigmaNew = dotProduct(z, r);

		double beta = sigmaNew / rho;

		GridData betaTimesS; betaTimesS.initialize();
		multiply(beta, s, betaTimesS);
		add(z, betaTimesS, s);
		//s = z + beta * s;

		sigma = sigmaNew;
	}

	PRINT_LINE( "PCG didn't converge!" );
	return false;

}


void MACGrid::calculatePreconditioner(const GridDataMatrix & A) {
    double tau=0.97;
	precon.initialize();
    FOR_EACH_CELL
            {
                double Aii = (i==0)?0:AMatrix.plusI(i-1,j,k);
                double Aij = (j==0)?0:AMatrix.plusI(i,j-1,k);
                double Aik = (k==0)?0:AMatrix.plusI(i,j,k-1);
                double Aji = (i==0)?0:AMatrix.plusJ(i-1,j,k);
                double Ajj = (j==0)?0:AMatrix.plusJ(i,j-1,k);
                double Ajk = (k==0)?0:AMatrix.plusJ(i,j,k-1);
                double Aki = (i==0)?0:AMatrix.plusK(i-1,j,k);
                double Akj = (j==0)?0:AMatrix.plusK(i,j-1,k);
                double Akk = (k==0)?0:AMatrix.plusK(i,j,k-1);

                double pri = (i==0)?0:precon(i-1,j,k);
                double prj = (j==0)?0:precon(i,j-1,k);
                double prk = (k==0)?0:precon(i,j,k-1);
                double e = AMatrix.diag(i,j,k) - (Aii*pri)*(Aii*pri) - (Ajj*prj)*(Ajj*prj) - (Akk*prk)*(Akk*prk) - tau*(Aii*(Aji+Aki)*pri*pri + Ajj*(Aij+Akj)*prj*prj + Akk*(Aik+Ajk)*prk*prk);
                precon(i,j,k) = 1/sqrt(e + 10e-30);
//                cout<<precon(i,j,k)<<endl;
            };

    // TODO: Build the modified incomplete Cholesky preconditioner following Fig 4.2 on page 36 of Bridson's 2007 SIGGRAPH fluid course notes.
    //       This corresponds to filling in precon(i,j,k) for all cells

}


void MACGrid::applyPreconditioner(const GridData & r, const GridDataMatrix & A, GridData & z) {

    // TODO: change if(0) to if(1) after you implement calculatePreconditoner function.

    if(1) {

        // APPLY THE PRECONDITIONER:
        // Solve Lq = r for q:
        GridData q;

        q.initialize();



        FOR_EACH_CELL {

                    //if (A.diag(i,j,k) != 0.0) { // If cell is a fluid.
                    double t = r(i, j, k) - A.plusI(i - 1, j, k) * precon(i - 1, j, k) * q(i - 1, j, k)
                               - A.plusJ(i, j - 1, k) * precon(i, j - 1, k) * q(i, j - 1, k)
                               - A.plusK(i, j, k - 1) * precon(i, j, k - 1) * q(i, j, k - 1);
                    q(i, j, k) = t * precon(i, j, k);
//                    cout<<t<<endl;
                    //}
                }
        // Solve L^Tz = q for z:
        FOR_EACH_CELL_REVERSE {
                    //if (A.diag(i,j,k) != 0.0) { // If cell is a fluid.
                    double t = q(i, j, k) - A.plusI(i, j, k) * precon(i, j, k) * z(i + 1, j, k)
                               - A.plusJ(i, j, k) * precon(i, j, k) * z(i, j + 1, k)
                               - A.plusK(i, j, k) * precon(i, j, k) * z(i, j, k + 1);
                    z(i, j, k) = t * precon(i, j, k);
//                    cout<<q(i,j,k)<<endl;
                }
    }
    else{
        // Unpreconditioned CG: Bypass preconditioner:
        z = r;
        return;
    }

}



double MACGrid::dotProduct(const GridData & vector1, const GridData & vector2) {
	
	double result = 0.0;

	FOR_EACH_CELL {
		result += vector1(i,j,k) * vector2(i,j,k);
	}

	return result;
}


void MACGrid::add(const GridData & vector1, const GridData & vector2, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = vector1(i,j,k) + vector2(i,j,k);
	}

}


void MACGrid::subtract(const GridData & vector1, const GridData & vector2, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = vector1(i,j,k) - vector2(i,j,k);
	}

}


void MACGrid::multiply(const double scalar, const GridData & vector, GridData & result) {
	
	FOR_EACH_CELL {
		result(i,j,k) = scalar * vector(i,j,k);
	}

}


double MACGrid::maxMagnitude(const GridData & vector) {
	
	double result = 0.0;

	FOR_EACH_CELL {
		if (abs(vector(i,j,k)) > result) result = abs(vector(i,j,k));
	}

	return result;
}


void MACGrid::apply(const GridDataMatrix & matrix, const GridData & vector, GridData & result) {
	
	FOR_EACH_CELL { // For each row of the matrix.

		double diag = 0;
		double plusI = 0;
		double plusJ = 0;
		double plusK = 0;
		double minusI = 0;
		double minusJ = 0;
		double minusK = 0;

		diag = matrix.diag(i,j,k) * vector(i,j,k);
		if (isValidCell(i+1,j,k)) plusI = matrix.plusI(i,j,k) * vector(i+1,j,k);
		if (isValidCell(i,j+1,k)) plusJ = matrix.plusJ(i,j,k) * vector(i,j+1,k);
		if (isValidCell(i,j,k+1)) plusK = matrix.plusK(i,j,k) * vector(i,j,k+1);
		if (isValidCell(i-1,j,k)) minusI = matrix.plusI(i-1,j,k) * vector(i-1,j,k);
		if (isValidCell(i,j-1,k)) minusJ = matrix.plusJ(i,j-1,k) * vector(i,j-1,k);
		if (isValidCell(i,j,k-1)) minusK = matrix.plusK(i,j,k-1) * vector(i,j,k-1);

		result(i,j,k) = diag + plusI + plusJ + plusK + minusI + minusJ + minusK;
	}

}

void MACGrid::saveSmoke(const char* fileName) {
	std::ofstream fileOut(fileName);
	if (fileOut.is_open()) {
		FOR_EACH_CELL {
			fileOut << mD(i,j,k) << std::endl;
		}
		fileOut.close();
	}
}

void MACGrid::saveParticle(std::string filename){
	Partio::ParticlesDataMutable *parts = Partio::create();
	Partio::ParticleAttribute posH, vH;
	posH = parts->addAttribute("position", Partio::VECTOR, 3);
	vH = parts->addAttribute("v", Partio::VECTOR, 3);
	for (unsigned int i = 0; i < rendering_particles.size(); i++)
	{
		int idx = parts->addParticle();
		float *p = parts->dataWrite<float>(posH, idx);
		float *v = parts->dataWrite<float>(vH, idx);
		for (int k = 0; k < 3; k++)
		{
			p[k] = rendering_particles[i][k];
			v[k] = rendering_particles_vel[i][k];
		}
	}
	
	Partio::write(filename.c_str(), *parts);
	parts->release();
}

void MACGrid::saveDensity(std::string filename){
	Partio::ParticlesDataMutable *density_field = Partio::create();
	Partio::ParticleAttribute posH, rhoH;
	posH = density_field->addAttribute("position", Partio::VECTOR, 3);
	rhoH = density_field->addAttribute("density", Partio::VECTOR, 1);
	FOR_EACH_CELL{
		int idx = density_field->addParticle();
		float *p = density_field->dataWrite<float>(posH, idx);
		float *rho = density_field->dataWrite<float>(rhoH, idx);
		vec3 cellCenter = getCenter(i, j, k);
		for (int l = 0; l < 3; l++)
		{
			p[l] = cellCenter[l];
		}
		rho[0] = getDensity(cellCenter);
	}
	Partio::write(filename.c_str(), *density_field);
	density_field->release();
}

void MACGrid::draw(const Camera& c)
{   
   drawWireGrid();
   if (theDisplayVel) drawVelocities();   
   if (theRenderMode == CUBES) drawSmokeCubes(c);
   else drawSmoke(c);
}

void MACGrid::drawVelocities()
{
   // draw line at each center
   //glColor4f(0.0, 1.0, 0.0, 1.0);
   glBegin(GL_LINES);
      FOR_EACH_CELL
      {
         vec3 pos = getCenter(i,j,k);
         vec3 vel = getVelocity(pos);
         if (vel.Length() > 0.0001)
         {
           //vel.Normalize(); 
           vel *= theCellSize/2.0;
           vel += pos;
		   glColor4f(1.0, 1.0, 0.0, 1.0);
           glVertex3dv(pos.n);
		   glColor4f(0.0, 1.0, 0.0, 1.0);
           glVertex3dv(vel.n);
         }
      }
   glEnd();
}

vec4 MACGrid::getRenderColor(int i, int j, int k)
{
	
	double value = mD(i, j, k); 
	vec4 coldColor(0.5, 0.5, 1.0, value);
	vec4 hotColor(1.0, 0.5, 0.5, value);
    return LERP(coldColor, hotColor, mT(i, j, k));
	

	/*
	// OLD:
    double value = mD(i, j, k); 
    return vec4(1.0, 0.9, 1.0, value);
	*/
}

vec4 MACGrid::getRenderColor(const vec3& pt)
{
	double value = getDensity(pt);
	vec4 coldColor(0.5, 0.5, 1.0, value);
	vec4 hotColor(1.0, 0.5, 0.5, value);
    return LERP(coldColor, hotColor, getTemperature(pt));

	/*
	// OLD:
    double value = getDensity(pt); 
    return vec4(1.0, 1.0, 1.0, value);
	*/
}

void MACGrid::drawZSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double startk = back - stepsize;
   double endk = 0;
   double stepk = -theCellSize;

   if (!backToFront)
   {
      startk = 0;
      endk = back;   
      stepk = theCellSize;
   }

   for (double k = startk; backToFront? k > endk : k < endk; k += stepk)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double i = 0.0; i <= right; i += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double i = right; i >= 0.0; i -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}

void MACGrid::drawXSheets(bool backToFront)
{
   // Draw K Sheets from back to front
   double back =  (theDim[2])*theCellSize;
   double top  =  (theDim[1])*theCellSize;
   double right = (theDim[0])*theCellSize;
  
   double stepsize = theCellSize*0.25;

   double starti = right - stepsize;
   double endi = 0;
   double stepi = -theCellSize;

   if (!backToFront)
   {
      starti = 0;
      endi = right;   
      stepi = theCellSize;
   }

   for (double i = starti; backToFront? i > endi : i < endi; i += stepi)
   {
     for (double j = 0.0; j < top; )
      {
         glBegin(GL_QUAD_STRIP);
         for (double k = 0.0; k <= back; k += stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;

         glBegin(GL_QUAD_STRIP);
         for (double k = back; k >= 0.0; k -= stepsize)
         {
            vec3 pos1 = vec3(i,j,k); 
            vec3 pos2 = vec3(i, j+stepsize, k); 

            vec4 color1 = getRenderColor(pos1);
            vec4 color2 = getRenderColor(pos2);

            glColor4dv(color1.n);
            glVertex3dv(pos1.n);

            glColor4dv(color2.n);
            glVertex3dv(pos2.n);
         } 
         glEnd();
         j+=stepsize;
      }
   }
}


void MACGrid::drawSmoke(const Camera& c)
{
   vec3 eyeDir = c.getBackward();
   double zresult = fabs(Dot(eyeDir, vec3(1,0,0)));
   double xresult = fabs(Dot(eyeDir, vec3(0,0,1)));
   //double yresult = fabs(Dot(eyeDir, vec3(0,1,0)));

   if (zresult < xresult)
   {      
      drawZSheets(c.getPosition()[2] < 0);
   }
   else 
   {
      drawXSheets(c.getPosition()[0] < 0);
   }
}

void MACGrid::drawSmokeCubes(const Camera& c)
{
   std::multimap<double, MACGrid::Cube, std::greater<double> > cubes;
   FOR_EACH_CELL
   {
      MACGrid::Cube cube;
      cube.color = getRenderColor(i,j,k);
      cube.pos = getCenter(i,j,k);
      cube.dist = DistanceSqr(cube.pos, c.getPosition());
      cubes.insert(make_pair(cube.dist, cube));
   } 

   // Draw cubes from back to front
   std::multimap<double, MACGrid::Cube, std::greater<double> >::const_iterator it;
   for (it = cubes.begin(); it != cubes.end(); ++it)
   {
      drawCube(it->second);
   }
}

void MACGrid::drawWireGrid()
{
   // Display grid in light grey, draw top & bottom

   double xstart = 0.0;
   double ystart = 0.0;
   double zstart = 0.0;
   double xend = theDim[0]*theCellSize;
   double yend = theDim[1]*theCellSize;
   double zend = theDim[2]*theCellSize;

   glPushAttrib(GL_LIGHTING_BIT | GL_LINE_BIT);
      glDisable(GL_LIGHTING);
      glColor3f(0.25, 0.25, 0.25);

      glBegin(GL_LINES);
      for (int i = 0; i <= theDim[0]; i++)
      {
         double x = xstart + i*theCellSize;
         glVertex3d(x, ystart, zstart);
         glVertex3d(x, ystart, zend);

         glVertex3d(x, yend, zstart);
         glVertex3d(x, yend, zend);
      }

      for (int i = 0; i <= theDim[2]; i++)
      {
         double z = zstart + i*theCellSize;
         glVertex3d(xstart, ystart, z);
         glVertex3d(xend, ystart, z);

         glVertex3d(xstart, yend, z);
         glVertex3d(xend, yend, z);
      }

      glVertex3d(xstart, ystart, zstart);
      glVertex3d(xstart, yend, zstart);

      glVertex3d(xend, ystart, zstart);
      glVertex3d(xend, yend, zstart);

      glVertex3d(xstart, ystart, zend);
      glVertex3d(xstart, yend, zend);

      glVertex3d(xend, ystart, zend);
      glVertex3d(xend, yend, zend);
      glEnd();
   glPopAttrib();

   glEnd();
}

#define LEN 0.5
void MACGrid::drawFace(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);
      glEnd();
   glPopMatrix();
}

void MACGrid::drawCube(const MACGrid::Cube& cube)
{
   glColor4dv(cube.color.n);
   glPushMatrix();
      glTranslated(cube.pos[0], cube.pos[1], cube.pos[2]);      
      glScaled(theCellSize, theCellSize, theCellSize);
      glBegin(GL_QUADS);
         glNormal3d( 0.0, -1.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN,  LEN);
         glVertex3d( LEN, -LEN, -LEN);         

         glNormal3d( 0.0,  0.0, -0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN,  LEN, -LEN);
         glVertex3d( LEN,  LEN, -LEN);
         glVertex3d( LEN, -LEN, -LEN);

         glNormal3d(-1.0,  0.0,  0.0);
         glVertex3d(-LEN, -LEN, -LEN);
         glVertex3d(-LEN, -LEN,  LEN);
         glVertex3d(-LEN,  LEN,  LEN);
         glVertex3d(-LEN,  LEN, -LEN);

         glNormal3d( 0.0, 1.0,  0.0);
         glVertex3d(-LEN, LEN, -LEN);
         glVertex3d(-LEN, LEN,  LEN);
         glVertex3d( LEN, LEN,  LEN);
         glVertex3d( LEN, LEN, -LEN);

         glNormal3d( 0.0,  0.0, 1.0);
         glVertex3d(-LEN, -LEN, LEN);
         glVertex3d(-LEN,  LEN, LEN);
         glVertex3d( LEN,  LEN, LEN);
         glVertex3d( LEN, -LEN, LEN);

         glNormal3d(1.0,  0.0,  0.0);
         glVertex3d(LEN, -LEN, -LEN);
         glVertex3d(LEN, -LEN,  LEN);
         glVertex3d(LEN,  LEN,  LEN);
         glVertex3d(LEN,  LEN, -LEN);
      glEnd();
   glPopMatrix();
}