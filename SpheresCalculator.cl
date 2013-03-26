#define _3D_ x
#define _double_ x
#define _G_ 1
#define _v_nicht_const_ 1
// Heun-Verfahren / Halbschritt-Verfahren
#define heun 1
#define step (5.0f/10.0f)
// Leapfrog-Verfahren
#define leapfrog 0

#if leapfrog && heun
	#error Error: only leapfrog OR heun possible!
#endif

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
//#pragma OPENCL EXTENSION cl_amd_fp64 : enable
//#pragma OPENCL EXTENSION cl_amd_vec3 : enable

//#define reduced 0.9999999999999
//0.9

#define double_vector double3
#define float_vector float3

#if _double_
	#define scalar double
	#define vector double_vector
	#define vector3 double3
	#define vector2 double2
#else
	#define scalar float
	#define vector float_vector
	#define vector3 float3
	#define vector2 float2
#endif

#define sqrt_(x) (sqrt(x))
#define pow_(x,y) (pow((scalar)(x),(scalar)(y)))
//sqrt half_sqrt native_sqrt

typedef struct Sphere
{
	vector pos, oldPos;
	vector speed;
	vector force;
	scalar size,mass,poisson,E;
	unsigned short fixed; // 0 or 1
	//float x[3];  //padding
} Sphere;

uint GetUint(uint* m_z, uint* m_w, uint gid)
{
	uint z = *m_z;
	z = 36969 * (z & 65535) + (z >> 16);
	*m_z = z;
	uint w = *m_w;
	w = 18000 * (w & 65535) + (w >> 16);
	*m_w = w;
	return (z << 16) + w;
}
uint gGetUint(__global uint* m_z, __global uint* m_w, uint gid)
{
	uint z = *m_z;
	z = 36969 * (z & 65535) + (z >> 16);
	*m_z = z;
	uint w = *m_w;
	w = 18000 * (w & 65535) + (w >> 16);
	*m_w = w;
	return (z << 16) + w;
}

scalar uGetUniform(uint* m_z, uint* m_w, uint gid)
{
	// 0 <= u < 2^32
	uint u = GetUint(m_z, m_w, gid);
	// The magic number below is 1/(2^32 + 2).
	// The result is strictly between 0 and 1.
	return (u + 1.0) //* 2.328306435454494e-10;
		* 2328306435454494 * pow_(10.0,-25.0);
}

scalar GetUniform(uint* m_z, uint* m_w, uint gid)
{
	// 0 <= u < 2^32
	uint u = GetUint(m_z, m_w, gid);
	// The magic number below is 1/(2^32 + 2).
	// The result is strictly between -1 and 1.
	return ((u + 1.0) //* 2.328306435454494e-10
		* 2328306435454494 * pow_(10.0,-25.0) 
		* 2)-1;
}


__kernel void randomFill(__global struct Sphere* sphere, __global uint* z, __global uint* w,
						__global vector3* boxSize, __global scalar* max_speed,
						__global vector* sphereSize, __global scalar* poisson,
						__global scalar* E, __global uint* num, __global int* flags){
	int gid = get_global_id(0);
	flags[gid] = 0;
	vector3 s = *boxSize;
	vector s2 = *sphereSize;
	uint m_z_ = 0;//(*z) + (gid*s.s0*1000000);
	uint m_w_ = 0;//(*w) + (gid*s2.s0*100000000);
	uint n = *num;
	for(int i = 0; i<n; i++){
		if(i == gid){
			m_z_ = gGetUint(z,w,gid);
			m_w_ = gGetUint(z,w,gid);
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}
	uint* m_z = &m_z_;
	uint* m_w = &m_w_;
	sphere[gid].size = s2.s0+((s2.s1-s2.s0)*uGetUniform(m_z, m_w, gid));
#ifndef M_PI
	float M_PI = 314/100.0f;
#endif
	sphere[gid].mass = 4.0/3.0*pow_(sphere[gid].size,3)*M_PI  *950; //Kautschuk
	sphere[gid].poisson = *poisson;
	sphere[gid].E = *E;
	   
	sphere[gid].pos.s0 = sphere[gid].size/2+uGetUniform(m_z, m_w, gid)*(s.s0-sphere[gid].size);
	sphere[gid].pos.s1 = sphere[gid].size/2+uGetUniform(m_z, m_w, gid)*(s.s1-sphere[gid].size);
	#if _3D_
		sphere[gid].pos.s2 = sphere[gid].size/2+uGetUniform(m_z, m_w, gid)*(s.s2-sphere[gid].size);
		//printf("pos: %f\n", sphere[gid].pos.s2);
	#endif
	sphere[gid].oldPos = sphere[gid].pos;
	#if leapfrog
		//sphere[gid].oldPos = 0;
	#endif
	
	sphere[gid].speed.s0 = GetUniform(m_z, m_w, gid);
	sphere[gid].speed.s1 = GetUniform(m_z, m_w, gid);
	#if _3D_
		sphere[gid].speed.s2 = GetUniform(m_z, m_w, gid);
	#endif
	sphere[gid].speed = normalize(sphere[gid].speed)*uGetUniform(m_z, m_w, gid)*(*max_speed);
	
	sphere[gid].force.s0 = 0;
	sphere[gid].force.s1 = 0;
	#if _3D_
		sphere[gid].force.s2 = 0;
	#endif
}

__kernel void moveStep2(__global struct Sphere* sphere, __global int* num,
						__global vector* sphereSize,
						__global scalar* elastic, __global vector* g,
						__global scalar* delta_t_, __global scalar* G_)
{
	vector s = *sphereSize, force, acceleration, pos, pos2, gravity = *g;
	scalar reduced = *elastic, delta_t = *delta_t_, G = *G_;
	int id = get_global_id(0);
	__global struct Sphere* c2;
	
	__global struct Sphere* c = &sphere[id];
	#if heun
		pos = c->pos + c->speed*(delta_t*step) + c->force/c->mass*(delta_t*step*delta_t*step)/2.0f;
	#endif
	
	vector d_pos, d_n;
	scalar both_r, d, d_, R, E_, force_;
	//*
	int n = *num, n2 = 0;
	for(int i = n-1; i>id; i--){
		c2 = &sphere[i];
		#if heun
			pos2 = c2->pos + c2->speed*(delta_t*step) + c2->force/c2->mass*(delta_t*step*delta_t*step)/2.0f;
		#endif
		
		//#define c2 sphere[i]
		both_r = c->size + c2->size;
		d_pos = pos2-pos;
		
		/*
		if(G == 0){
			if(d_pos.s0>both_r||d_pos.s0<-both_r||d_pos.s1>both_r||d_pos.s1<-both_r
			#if _3D_
			||d_pos.s2>both_r||d_pos.s2<-both_r
			#endif
			){
				continue;
			}
			//*
			else{
				if(++n2>6) break;
			}// * /
		}// */

		//d_pos.s2 = 0;
		d = length(d_pos);
		
		if(G!=0){
			d_n = d_pos/d; // bzw. normalize(d_pos);
			// Gravitation:
			force = G*c->mass*c2->mass/pow_(d,2) *d_n;
			sphere[id].force += force;
			sphere[i].force -= force;
		}
		
		// Abstossung:
		if (d < both_r) {
			if(G == 0){
				d_n = d_pos/d; // bzw. normalize(d_pos);
			}
			// nach Kontaktmechanik mit Poisson-Zahlen:
			d_ = both_r - d;
			R = 1/((1/c->size)+(1/c2->size));
			E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c2->poisson*c2->poisson)/c2->E));
			force = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3)) *d_n;
			if(dot(d_pos, (c->speed-c2->speed)/d)<0){ //Skalarprodukt
				force *= reduced;
			}
			sphere[id].force -= force;
			sphere[i].force += force;
			//continue;
			
			/*
			//c->thisStepHit = true;
			//if(!(c_.free && c2->free))continue;
			d_d = (both_r/d-1)*e*d_pos;
			// *
			if(dot(d_pos, (c->speed-c2->speed)/d)<0){ //Skalarprodukt
				d_d *= reduced;
				//sphere[id].speed = 0;
				//sphere[i].speed = 0;
			}// * /
			sphere[id].force -= c->size*d_d;
			sphere[i].force += c2->size*d_d;
			
			/ *
			htc_d = sqrt(htc_dSqr);
			htc_d_d = htc_both_r-htc_d;
			htc_f_1 = E*c_.r*c_.r*htc_d_d/(2*c_.r)/1000000.0;
			htc_f_2 = E*c2->r*c2->r*htc_d_d/(2*c2->r)/1000000.0;
			c->fx = c_.fx-htc_dx/htc_d*htc_f_1;
			c->fy = c_.fy-htc_dy/htc_d*htc_f_1;
			c2->fx += htc_dx/htc_d*htc_f_2;
			c2->fy += htc_dy/htc_d*htc_f_2;// */
		}
	}// */
	
	//*
	scalar htw_d_d, fact = 1.0f;
	if ((htw_d_d = (c->size - pos.s0))>0) {
		//c->speed.s0 = fabs(c->speed.s0);
		if(c->speed.s0 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s0 += force_*fact;
		//sphere[id].force.s0 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + pos.s0 - s.s0))>0) {
		//c->speed.s0 = -fabs(c->speed.s0);
		if(c->speed.s0 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s0 -= force_*fact;
		//sphere[id].force.s0 -= c->size*htw_d_d*c->E*fact;
	}
	fact = 1.0f;
	if ((htw_d_d = (c->size - pos.s1))>0) {
		//c->speed.s1 = fabs(c->speed.s1);
		if(c->speed.s1 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s1 += force_*fact;
		//sphere[id].force.s1 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + pos.s1 - s.s1))>0) {
		//c->speed.s1 = -fabs(c->speed.s1);
		if(c->speed.s1 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s1 -= force_*fact;
		//sphere[id].force.s1 -= c->size*htw_d_d*c->E*fact;
	}
#if _3D_
	fact = 1.0f;
	if ((htw_d_d = (c->size - pos.s2))>0) {
		if(c->speed.s2 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s2 += force_*fact;
		//sphere[id].force.s2 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + pos.s2 - s.s2))>0) {
		if(c->speed.s2 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s2 -= force_*fact;
		//sphere[id].force.s2 -= c->size*htw_d_d*c->E*fact;
	}
#endif
	// */
	
	// Luftwiderstand:
	//sphere[id].force -= 0.001*sphere[id].speed*fabs(length(sphere[id].speed));
	//0.00001*normalize(sphere[id].speed)*pow_(length(sphere[id].speed),2);
	
	/*sphere[id].pos += c->speed;
	force = sphere[id].force;
	sphere[id].speed += force;
	sphere[id].force -= force;*/
	
	force = c->force;
	
	
	// Gravitation nach unten:
	//sphere[id].force -= gravity*sphere[id].mass;
	acceleration = force / c->mass;
	acceleration -= gravity;
	
	c->pos += c->speed*delta_t;
	#if _v_nicht_const_
		c->pos += acceleration*(delta_t*delta_t)/2.0f;
	#endif
	c->speed += acceleration*delta_t;
	c->force -= force;
}

#define addForce(i,f) {						\
	while (atomic_cmpxchg(&flags[i],0,1)==1);\
	sphere[i].force += f;					\
	atomic_xchg(&flags[i],0);							\
}

void GetSemaphor_old(__global int * semaphor) {
   int occupied = atomic_xchg(semaphor, 1);
   while(occupied == 1)
   //for(int i = 0; i<1000 && occupied > 0; i++)
   {
	 occupied = atomic_xchg(semaphor, 1);
   }
}
#define addForce2(i,f) {						\
	int waiting = 1;							\
	while(waiting){								\
		if(atomic_xchg(&flags[i], 1)==0){		\
			sphere[i].force += f;				\
			atomic_xchg(&flags[i],0);			\
			waiting = 0;						\
		}										\
	}											\
}

 

void ReleaseSemaphor(__global int * semaphor)
{
   int prevVal = atomic_xchg(semaphor, 0);
}

__kernel void moveStep3_addInterForces(__global struct Sphere* sphere, 
						__global scalar* elastic, __global scalar* G_,
						__global int* flags)
{
	int id = get_global_id(0);
	int id2 = get_global_id(1);
	//printf("group id: %5d local id: %5d global id: %5d\n",id,id2, get_global_id(0));
	
		//flags[id] = 0;
		
		//flags[id2] = 0;
	
		//barrier(CLK_GLOBAL_MEM_FENCE);
		
	if(id2>=id) return;
		
	__global struct Sphere* c;
	c = &sphere[id];
	__global struct Sphere* c2;
	c2 = &sphere[id2];
	
	vector force;
	scalar reduced = *elastic, G = *G_;
	
	vector d_pos, d_n;
	scalar both_r, d, d_, R, E_;
	
	//#define c2 sphere[i]
	both_r = c->size + c2->size;
	d_pos = c2->pos-c->pos;
	
	//*
	#if _G_==0
	if(G == 0)
	{
		if(d_pos.s0>both_r||d_pos.s0<-both_r||d_pos.s1>both_r||d_pos.s1<-both_r
			#if _3D_
			||d_pos.s2>both_r||d_pos.s2<-both_r
			#endif
			){
			return;
		}
	}
	#endif
	// */
	
	//d_pos.s2 = 0;
	d = length(d_pos);
	//d = max(d,(scalar)0.00005f);
	
	/*
	d_n = d_pos/d;
	force = 10000000000000000000000000.0*pow_(d,12) *d_n;
	c->force -= force;
	c2->force += force;
	force = 1000000000000.0*pow_(d,6) *d_n;
	c->force += force;
	c2->force -= force;
	return;*/
	
	#if _G_
	if(G!=0)
	{
		d_n = d_pos/d;
		// bzw. normalize(d_pos);
		// Gravitation:
		force = G*c->mass*c2->mass/pow_(max(d,(scalar)c->size/10),2) *d_n;
		c->force += force;
		
		c2->force -= force;
	}
	#endif
	
	// Abstossung:
	if (d < both_r) {
		//*
		//#if _G_==0
		if(G == 0)
		{
			d_n = d_pos/d; // bzw. normalize(d_pos);
		}
		//#endif
		// */
		// nach Kontaktmechanik mit Poisson-Zahlen:
		d_ = both_r - d;
		//*
		R = 1/((1/c->size)+(1/c2->size));
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c2->poisson*c2->poisson)/c2->E));
		force = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3)) *d_n;
		if(reduced!=1 && dot(d_pos, (c->speed-c2->speed)/d)<0){ //Skalarprodukt
			force *= reduced;
		}
		//printf("id: %d id2: %d index1: %d index2: %d\n", id, id2, indices[id], indices[id2]);
		//GetSemaphor(&flags[id]);
		//c->force -= force;
		//ReleaseSemaphor(&flags[id]);
		addForce2(id,-force);
		//GetSemaphor(&flags[id2]);
		//c2->force += force;
		//ReleaseSemaphor(&flags[id2]);
		addForce2(id2,force);
		// */
		//printf("now: id: %d id2: %d index1: %d index2: %d\n", id, id2, indices[id], indices[id2]);
	}
	//printf("id: %5d id2: %5d\n",id,id2);
}


__kernel void randomFill2(__global struct Sphere* sphere, 
						__global vector* sphereSize,
						__global scalar* elastic){
}

__kernel void moveStep3_addWallForces(__global struct Sphere* sphere, 
						__global vector* sphereSize,
						__global scalar* elastic)
{
	vector s = *sphereSize;
	scalar reduced = *elastic;
	int id = get_global_id(0);
	
	__global struct Sphere* c = &sphere[id];
	
	scalar d_, R, E_, force_;
	
	//*
	scalar htw_d_d, fact = 1.0f;
	if ((htw_d_d = (c->size - c->pos.s0))>0) {
		//c->speed.s0 = fabs(c->speed.s0);
		if(c->speed.s0 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s0 += force_*fact;
		//sphere[id].force.s0 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + c->pos.s0 - s.s0))>0) {
		//c->speed.s0 = -fabs(c->speed.s0);
		if(c->speed.s0 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s0 -= force_*fact;
		//sphere[id].force.s0 -= c->size*htw_d_d*c->E*fact;
	}
	fact = 1.0f;
	if ((htw_d_d = (c->size - c->pos.s1))>0) {
		//c->speed.s1 = fabs(c->speed.s1);
		if(c->speed.s1 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s1 += force_*fact;
		//sphere[id].force.s1 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + c->pos.s1 - s.s1))>0) {
		//c->speed.s1 = -fabs(c->speed.s1);
		if(c->speed.s1 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s1 -= force_*fact;
		//sphere[id].force.s1 -= c->size*htw_d_d*c->E*fact;
	}
#if _3D_
	fact = 1.0f;
	if ((htw_d_d = (c->size - c->pos.s2))>0) {
		if(c->speed.s2 > 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s2 += force_*fact;
		//sphere[id].force.s2 += c->size*htw_d_d*c->E*fact;
	}else if ((htw_d_d = (c->size + c->pos.s2 - s.s2))>0) {
		if(c->speed.s2 < 0)fact = reduced;
		d_ = htw_d_d;
		R = c->size;
		E_ = 1/(((1-c->poisson*c->poisson)/c->E)+((1-c->poisson*c->poisson)/c->E));
		force_ = 4.0f/3.0f*E_*sqrt(R*pow_(d_,3));
		sphere[id].force.s2 -= force_*fact;
		//sphere[id].force.s2 -= c->size*htw_d_d*c->E*fact;
	}
#endif
}


__kernel void moveStep3_updatePositions(__global struct Sphere* sphere,
						__global vector* g,
						__global scalar* delta_t_)
{
	vector force, acceleration, gravity = *g;
	scalar delta_t = *delta_t_;
	int id = get_global_id(0);
	
	__global struct Sphere* c = &sphere[id];
	
	// */
	
	// Luftwiderstand:
	//sphere[id].force -= 0.001*sphere[id].speed*fabs(length(sphere[id].speed));
	//0.00001*normalize(sphere[id].speed)*pow_(length(sphere[id].speed),2);
	
	/*sphere[id].pos += c->speed;
	force = sphere[id].force;
	sphere[id].speed += force;
	sphere[id].force -= force;*/
	
	force = c->force*1;
	
	
	// Gravitation nach unten:
	//sphere[id].force -= gravity*sphere[id].mass;
	acceleration = force / c->mass;
	acceleration += gravity;
	
	#if leapfrog
	
		vector pos = c->pos;
		c->pos = 2*c->pos - c->oldPos + acceleration*delta_t*delta_t;
		c->oldPos = pos;
		
		//c->pos = c->pos + c->speed*delta_t + 1.0f/2.0f*acceleration*delta_t*delta_t;
		//c->speed = c->speed + 1.0f/2.0f*(c->oldPos + acceleration)*delta_t;
		
		
		//c->oldPos = acceleration;
		
		c->force -= force;
	
	#else
		
		#if heun
			c->pos = c->oldPos;
		#endif
		c->pos += c->speed*delta_t;
		
		#if _v_nicht_const_
			c->pos += acceleration*(delta_t*delta_t)/2.0f;
		#endif
		c->speed += acceleration*delta_t;
		c->force -= force;
		
		#if heun
			c->oldPos = c->pos;
			c->pos += c->speed*(delta_t*step) + c->force/c->mass*(delta_t*step*delta_t*step)/2.0f;
		#endif
	
	#endif
}

