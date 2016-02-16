
#include "palabos2D.h"
#ifndef PLB_PRECOMPILED // Unless precompiled version is used,
#include "palabos2D.hh"   // include full template code
#endif
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace plb;
using namespace std;

namespace plb_acoustics{
	
	template<typename T, template<typename U> class Descriptor>
	void defineAnechoicWall(plint nx, plint ny,
	 MultiBlockLattice2D<T,Descriptor>& lattice,
	  T size_anechoic_buffer, plint orientation, T omega, 
	  Array<plint, 2> position_anechoic_wall, plint length_anechoic_wall,
	  T rhoBar_target, Array<T,2> j_target){
		
		// delta increase to the right
		if(orientation == 1){
			for(T delta = 0; delta <= size_anechoic_buffer; delta++){        
		        DotList2D points_to_aplly_dynamics;
		        for (int i = 0; i <= length_anechoic_wall; ++i){
		            points_to_aplly_dynamics.addDot(
		            	Dot2D(position_anechoic_wall[0] + delta,
		            	position_anechoic_wall[1] + i));
		        }
		        AnechoicDynamics<T,DESCRIPTOR> *anechoicDynamics = 
		        new AnechoicDynamics<T,DESCRIPTOR>(omega);
		        anechoicDynamics->setDelta((T) delta);
		        anechoicDynamics->setRhoBar_target(rhoBar_target);
		        j_target[0] = -j_target[0];  
		        anechoicDynamics->setJ_target(j_target);
		        defineDynamics(lattice, points_to_aplly_dynamics, anechoicDynamics);
	    	}
		}

		// delta increase to the top
		else if(orientation == 4){
			for(T delta = 0; delta <= size_anechoic_buffer; delta++){        
		        DotList2D points_to_aplly_dynamics;
		        for (int i = 0; i <= length_anechoic_wall; ++i){
		            points_to_aplly_dynamics.addDot(
		            	Dot2D(position_anechoic_wall[0] + i,
		            	position_anechoic_wall[1] + delta));
		        }
		        AnechoicDynamics<T,DESCRIPTOR> *anechoicDynamics = 
		        new AnechoicDynamics<T,DESCRIPTOR>(omega);
		        anechoicDynamics->setDelta((T) delta);
		        anechoicDynamics->setRhoBar_target(rhoBar_target);
		        

		        anechoicDynamics->setJ_target(j_target);
		        defineDynamics(lattice, points_to_aplly_dynamics, anechoicDynamics);
	    	}
		}

		// delta increase to the left
		else if(orientation == 3){
			for(T delta = 0; delta <= size_anechoic_buffer; delta++){        
		        DotList2D points_to_aplly_dynamics;
		        for (int i = 0; i <= length_anechoic_wall; ++i){
		            points_to_aplly_dynamics.addDot(
		            	Dot2D(position_anechoic_wall[0] + delta,
		            	position_anechoic_wall[1] + i));
		        }
		        AnechoicDynamics<T,DESCRIPTOR> *anechoicDynamics = 
		        new AnechoicDynamics<T,DESCRIPTOR>(omega);
		        T delta_left = 30 - delta;
		        anechoicDynamics->setDelta(delta_left);
		        anechoicDynamics->setRhoBar_target(rhoBar_target);
		        anechoicDynamics->setJ_target(j_target);
		        defineDynamics(lattice, points_to_aplly_dynamics, anechoicDynamics);
	    	}
		}

		// delta increase to the bottom
		else if(orientation == 2){
			for(T delta = 0; delta <= size_anechoic_buffer; delta++){        
		        DotList2D points_to_aplly_dynamics;
		        for (int i = 0; i <= length_anechoic_wall; ++i){
		            points_to_aplly_dynamics.addDot(
		            	Dot2D(position_anechoic_wall[0] + i,
		            	position_anechoic_wall[1] +  delta));
		        }
		        AnechoicDynamics<T,DESCRIPTOR> *anechoicDynamics = 
		        new AnechoicDynamics<T,DESCRIPTOR>(omega);
		        T delta_left = 30 - delta;
		        anechoicDynamics->setDelta(delta_left);
		        anechoicDynamics->setRhoBar_target(rhoBar_target);
		        anechoicDynamics->setJ_target(j_target);
		        defineDynamics(lattice, points_to_aplly_dynamics, anechoicDynamics);
	    	}
		}

		else{
			cout << "Anechoic Dynamics not Defined." << endl;
			cout << "Choose the correct orientation number." << endl;
		}

	}

	template<typename T, template<typename U> class Descriptor>
	void defineAnechoicWallOnTheRightSide(plint nx, plint ny,
	 MultiBlockLattice2D<T,Descriptor>& lattice, T size_anechoic_buffer, T omega){
	 	plint orientation = 1;
	    plint length_anechoic_wall = ny;
	    // position x and y
	    Array<plint, 2> position_anechoic_wall((plint) nx - size_anechoic_buffer - 1, 0);
	    defineAnechoicWall(nx, ny, lattice, size_anechoic_buffer,
	                       orientation, omega, position_anechoic_wall, length_anechoic_wall);
	}

	template<typename T, template<typename U> class Descriptor>
	void defineAnechoicWallOnTheLeftSide(plint nx, plint ny,
	 MultiBlockLattice2D<T,Descriptor>& lattice, T size_anechoic_buffer, T omega){
	 	plint orientation = 3;
	    plint length_anechoic_wall = ny;
	    // position x and y
	    Array<plint, 2> position_anechoic_wall(0, 0);
	    defineAnechoicWall(nx, ny, lattice, size_anechoic_buffer,
	                       orientation, omega, position_anechoic_wall, length_anechoic_wall);
	}

	template<typename T, template<typename U> class Descriptor>
	void defineAnechoicWallOnTheTopSide(plint nx, plint ny,
	 MultiBlockLattice2D<T,Descriptor>& lattice, T size_anechoic_buffer, T omega){
	 	plint orientation = 4;
	    plint length_anechoic_wall = nx;
	    // position x and y
	    Array<plint, 2> position_anechoic_wall(0, (plint) ny - size_anechoic_buffer - 1);
	    defineAnechoicWall(nx, ny, lattice, size_anechoic_buffer,
	                       orientation, omega, position_anechoic_wall, length_anechoic_wall);
	}

	template<typename T, template<typename U> class Descriptor>
	void defineAnechoicWallOnTheBottomSide(plint nx, plint ny,
	 MultiBlockLattice2D<T,Descriptor>& lattice, T size_anechoic_buffer, T omega){
	 	plint orientation = 2;
	    plint length_anechoic_wall = nx;
	    // position x and y
	    Array<plint, 2> position_anechoic_wall(0, 0);
	    defineAnechoicWall(nx, ny, lattice, size_anechoic_buffer,
	                       orientation, omega, position_anechoic_wall, length_anechoic_wall);
	}

}