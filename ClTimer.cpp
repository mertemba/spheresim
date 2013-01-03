
#include "ClTimer.h"

#include <QtGui/QApplication>
#include <cstdio>
//#include <stdio.h>
#include <cstdlib>
//#include <stdlib.h>
//#include <time.h>
#include <string>
#include <string.h>
#include <cmath>
#include "NanosecondTimer.h"

ClTimer::ClTimer(){
	try{
		printf("sizeof(Circle): %d\n", sizeof(Circle));
		srand(NanosecondTimer::getNS());
		
		if(useCircleExtensions){
			ceBuffer = new CircleExtension[circlesCount];
			for(int i = 0; i<circlesCount; i++){
				ceBuffer[i].color = (rand()%256)+(256*(rand()%256))+(256*256*(rand()%256));
				ceBuffer[i].trace = new cl_float2[traceCount];
				ceBuffer[i].traceCount = 0;
				ceBuffer[i].traceFull = false;
			}
		}
		
		
		glWidget = NULL;
		elapsedFrames = 0;
		printf("Initialize OpenCL object and context\n");
		//setup devices and context
		
		//this function is defined in util.cpp
		//it comes from the NVIDIA SDK example code
		///err = oclGetPlatformID(&platform);
		//oclErrorString is also defined in util.cpp and comes from the NVIDIA SDK
		///printf("oclGetPlatformID: %s\n", oclErrorString(err));
		std::vector<cl::Platform> platforms;
		err = cl::Platform::get(&platforms);
		printf("cl::Platform::get(): %s\n", (err));
		printf("numer of platforms: %d\n", platforms.size());
		if (platforms.size() == 0) {
			printf("Platform size 0\n");
		}
		
		//for right now we just use the first available device
		//later you may have criteria (such as support for different extensions)
		//that you want to use to select the device
		deviceUsed = 0;
		//create the context
		///context = clCreateContext(0, 1, &devices[deviceUsed], NULL, NULL, &err);
		//context properties will be important later, for now we go with defualts
		cl_context_properties properties[] = 
			{ CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};

		context = cl::Context(CL_DEVICE_TYPE_ALL, properties);
		devices = context.getInfo<CL_CONTEXT_DEVICES>();
		
		printf("number of devices: %d\n", devices.size());
		
		//create the command queue we will use to execute OpenCL commands
		///command_queue = clCreateCommandQueue(context, devices[deviceUsed], 0, &err);
		try{
			queue = cl::CommandQueue(context, devices[deviceUsed], 0, &err);
		}
		catch (cl::Error er) {
			printf("ERROR: %s(%d)\n", er.what(), er.err());
		}
		
		
		//Program Setup
		printf("load the program\n");
		

		int pl;
		char *kernel_source;
		kernel_source = file_contents("../CirclesCalculator.cl", &pl);
		if(kernel_source == NULL){
			printf("ERROR: OpenCL initialization failed!\n");
			return;
		}
		kernel_source[13] = '0'+_3D_; //define OpenCL version of _3D_

		//pl = kernel_source.boxSize();
		printf("kernel boxSize: %d\n", pl);
		//printf("kernel: \n %s\n", kernel_source.c_str());
		try
		{
			cl::Program::Sources source(1,
				std::make_pair(kernel_source, pl));
			program = cl::Program(context, source);
		}
		catch (cl::Error er) {
			printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
		}

		printf("build program\n");
		try
		{
			err = program.build(devices, "");//-cl-fast-relaxed-math -cl-single-precision-constant
		}
		catch (cl::Error er) {
			printf("program.build: %s\n", oclErrorString(er.err()));
		//if(err != CL_SUCCESS){
		}
		printf("done building program\n");
		printf("Build Status:  %u\n", program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]));
		printf("Build Options: %s\n", program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]).c_str());
		printf("Build Log:	 %s\n", program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]).c_str());
		printf("OpenCL initialization finished!\n");
		printf("\n");
		
		
		//initialize our kernel from the program
		try{
			moveStep_kernel = cl::Kernel(program, "moveStep", &err);
			randomFill_kernel = cl::Kernel(program, "randomFill", &err);
		}
		catch (cl::Error er) {
			printf("ERROR: %s(%d)\n", er.what(), er.err());
		}

		//initialize our CPU memory arrays, send them to the device and set the kernel arguements

		///Circle circle[circlesCount];
		//circle.A = .5f;
		//circle.B = 10.0f;
		//circle.C = 3;
		/*
		circle[i].D = .01;
		circle[i].E = 1;
		*/
		uint* m_z = new uint(rand());
		//for(int i = 0; i<10000; i++);
		uint* m_w = new uint(rand());
		cl_double2* boxSize2D = new cl_double2((cl_double2){boxSize.s0,boxSize.s1});
		
		int cl_mem_method = CL_MEM_COPY_HOST_PTR;
		//cl_mem_method = CL_MEM_USE_HOST_PTR;

		printf("Creating OpenCL arrays\n");
		//*
		if((cl_mem_method & CL_MEM_USE_HOST_PTR)>0){
			cl_circles = cl::Buffer(context, CL_MEM_READ_WRITE|cl_mem_method, sizeof(Circle)*circlesCount, circlesBuffer = new Circle[circlesCount], &err);
			if(err!=CL_SUCCESS)printf("ERROR: creating circles buffer: %s\n", oclErrorString(err));
			printf("circles buffer allocated.\n");
			circlesBufferUsed = true;
		}else // */
		{
			cl_circles = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(Circle)*circlesCount, NULL, &err);
			circlesBufferUsed = false;
		}
		cl_m_z = cl::Buffer(context, CL_MEM_READ_WRITE|cl_mem_method, sizeof(uint), m_z, &err);
		cl_m_w = cl::Buffer(context, CL_MEM_READ_WRITE|cl_mem_method, sizeof(uint), m_w, &err);
		cl_boxSize = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(cl_double2), boxSize2D, &err);
		cl_size = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(cl_double2), &size, &err);
		cl_max_speed = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &max_speed, &err);
		cl_circlesCount = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(int), &circlesCount, &err);
		cl_E = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &E, &err);
		cl_elastic = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &elastic, &err);
		cl_gravity = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &gravity, &err);
		cl_timeInterval = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &timeInterval, &err);
		cl_poisson = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &poisson, &err);
		cl_G = cl::Buffer(context, CL_MEM_READ_ONLY|cl_mem_method, sizeof(double), &G, &err);

		printf("Pushing data to the GPU\n");
		//push our CPU arrays to the GPU

		//write the circle struct to GPU memory as a buffer
		//err = queue.enqueueWriteBuffer(cl_circles, CL_TRUE, 0, sizeof(Circle), &circle, NULL, &event);
		/*err = queue.enqueueWriteBuffer(cl_m_z, CL_TRUE, 0, sizeof(uint), m_z, NULL, &event);
		err = queue.enqueueWriteBuffer(cl_m_w, CL_TRUE, 0, sizeof(uint), m_w, NULL, &event);
		err = queue.enqueueWriteBuffer(cl_boxSize, CL_TRUE, 0, sizeof(cl_double2), boxSize2D, NULL, &event);
		err = queue.enqueueWriteBuffer(cl_max_speed, CL_TRUE, 0, sizeof(double), &max_speed, NULL, &event);
		err = queue.enqueueWriteBuffer(cl_circlesCount, CL_TRUE, 0, sizeof(int), &circlesCount, NULL, &event);
		err = queue.enqueueWriteBuffer(cl_E, CL_TRUE, 0, sizeof(double), &E, NULL, &event);*/
		

		//Wait for the command queue to finish these commands before proceeding
		queue.finish();
	   


		//set the arguements of our kernel
		err = randomFill_kernel.setArg(0, cl_circles);
		err = randomFill_kernel.setArg(1, cl_m_z);
		err = randomFill_kernel.setArg(2, cl_m_w);
		err = randomFill_kernel.setArg(3, cl_boxSize);
		err = randomFill_kernel.setArg(4, cl_max_speed);
		err = randomFill_kernel.setArg(5, cl_size);
		err = randomFill_kernel.setArg(6, cl_poisson);
		err = randomFill_kernel.setArg(7, cl_E);
		
		err = moveStep_kernel.setArg(0, cl_circles);
		err = moveStep_kernel.setArg(1, cl_circlesCount);
		err = moveStep_kernel.setArg(2, cl_boxSize);
		err = moveStep_kernel.setArg(3, cl_elastic);
		err = moveStep_kernel.setArg(4, cl_gravity);
		err = moveStep_kernel.setArg(5, cl_timeInterval);
		err = moveStep_kernel.setArg(6, cl_G);

		//Wait for the command queue to finish these commands before proceeding
		queue.finish();
		
		
		
		printf("runKernel\n");
		events = new cl::Event[numEvents];
		eventCounter = 0;
		eventsFull = false;
		//std::vector<cl::Event> event2(1);
		if(saveBool){
			file = fopen("save.txt","w");
			fprintf(file, "%u ", _3D_);
			fprintf(file, "%g ", boxSize.s0);
			fprintf(file, "%g ", boxSize.s1);
			//#if _3D_
			//	f<<boxSize.s2<<" ";
			//#endif
			fprintf(file, "%u\n", circlesCount);
			fclose(file);
			
			readNum_save = min(1000,circlesCount/2);
			c_CPU_save[0] = new Circle[readNum_save];
			c_CPU_save[1] = new Circle[readNum_save];
		}
			
		err = queue.enqueueNDRangeKernel(randomFill_kernel, cl::NullRange, cl::NDRange(circlesCount), cl::NullRange, NULL, &event); 
		if(err!=CL_SUCCESS)printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
		queue.finish();
		printf("kernel randomFill executed successfully!\n\n");
		
		if(renderBool){
			//c_CPU_render[0] = new Circle[readNum_render];
			//c_CPU_render[1] = new Circle[readNum_render];
			readNum_render = min(1000,circlesCount);
			c_CPU_render = new Circle*[renderBufferCount];
			for(int i = 0; i<renderBufferCount; i++){
				c_CPU_render[i] = new Circle[readNum_render];
			}
			bufferReadIndex = 0;
			bufferWriteIndex = 0;
			err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, 0, sizeof(Circle)*readNum_render, c_CPU_render[bufferWriteIndex], NULL, NULL);//&event);
			bufferWriteIndex = ((bufferWriteIndex+1)%renderBufferCount);
		}
		
		/*double r,x,y,z;
		int j = 0;
		int offset = 0;
		Circle c;
		err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum), c_CPU_render[j], NULL, &event);
		//offset+=readNum;
		for(; offset<circlesCount; offset+=readNum){
			event.wait();
			if(circlesCount-(offset+readNum)>0){
				err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*(offset+readNum), sizeof(Circle)*min((circlesCount-(offset+readNum)),readNum), c_CPU_render[((j+1)%2)], NULL, &event);
			}
			//queue.finish();

			for(int i=0; i < min((circlesCount-offset),readNum); i++)
			{
				//printf("%4u: %5u\n", j, offset+i);
				c = c_CPU_render[j][i];
				r = c.size;
				x = c.pos.s0;
				y = c.pos.s1;
				z = 0;
				printf("Circle size(%3f) mass(%3f) E(%3f) ",r,c.mass,c.E);
				printf("pos(%4f|%4f|%4f) ",x,y,z);
				printf("speed(%4f|%4f|%4f) ",c.speed.s0,c.speed.s1,0.0);
				printf("force(%4f|%4f|%4f)\n",c.force.s0,c.force.s1,0.0);
			}
		}*/
		
		
		/*
		err = queue.enqueueNDRangeKernel(moveStep_kernel, cl::NullRange, cl::NDRange(circlesCount), cl::NullRange, NULL, NULL); 
		queue.finish();
		
		printf("after kernel moveStep:\n");
		
		j = 0;
		offset = 0;
		err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum), c_CPU_render[j], NULL, &event);
		//offset+=readNum;
		for(; offset<circlesCount; offset+=readNum){
			if(circlesCount-(offset+readNum)>0){
				err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*(offset+readNum), sizeof(Circle)*min((circlesCount-(offset+readNum)),readNum), c_CPU_render[((j+1)%2)], NULL, &event);
			}
			//queue.finish();

			for(int i=0; i < min((circlesCount-offset),readNum); i++)
			{
				//printf("%4u: %5u\n", j, offset+i);
				c = c_CPU_render[j][i];
				r = c.size;
				x = c.pos.s0;
				y = c.pos.s1;
				z = 0;
				printf("Circle size(%3f) mass(%3f) E(%3f) poisson(%3f) ",r,c.mass,c.E,c.poisson);
				printf("pos(%4f|%4f|%4f) ",x,y,z);
				printf("speed(%4f|%4f|%4f) ",c.speed.s0,c.speed.s1,0.0);
				printf("force(%4f|%4f|%4f)\n",c.force.s0,c.force.s1,0.0);
			}
		}*/
	}
	catch (cl::Error er) {
		printf("ERROR: %s(%d)\n", er.what(), er.err());
	}
}

void ClTimer::set(GLWidget* w){
	glWidget = w;
}

void ClTimer::fpsChanged(double fps){
	if(speed!=0 && fps!=0){
		double timeInterval = speed/fps;
		//printf("timeInterval: %10f\n", timeInterval);
		err = queue.enqueueWriteBuffer(cl_timeInterval, CL_TRUE, 0, sizeof(double), &timeInterval, NULL, &event);
	}
}

char ClTimer::hex(int i){
	if(i<0 || i>15){
		return '0';
	}else if(i<10){
		return '0'+i;
	}else{
		return 'A'+(i-10);
	}
}

void ClTimer::add(double d){
	unsigned char* c = (unsigned char*)&d;
	unsigned int x = 0;
	for(int i = 0; i<8; i++){
		fprintf(file, "%c%c", hex(c[i]/16), hex(c[i]%16));
	}
}

void ClTimer::save(){
	file = fopen("save.txt","a");
	int j = 0;
	int offset = 0;
	err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum_save), c_CPU_save[j=((j+1)%2)], NULL, &event);
	offset+=readNum_save;
	for(; offset<circlesCount; offset+=readNum_save){
		event.wait();
		err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum_save), c_CPU_save[j=((j+1)%2)], NULL, &event);
		//queue.finish();

		for(int i=0; i < min((circlesCount-offset),readNum_save); i++)
		{
			add(c_CPU_save[j][i].size);
			fprintf(file," ");
			add(c_CPU_save[j][i].pos.s0);
			fprintf(file," ");
			add(c_CPU_save[j][i].pos.s1);
			//#if _3D_
			//	f<<" ";
			//	add(f, c_CPU_save[j][i].pos.s2);
			//#endif
			fprintf(file,"\n");
		}
	}
	fclose(file);
}

double ClTimer::getFrameBufferLoad(){
	int ri = bufferReadIndex, wi = bufferWriteIndex;
	while(wi<ri){
		wi += renderBufferCount;
	}
	return (1.0*(wi-ri))/renderBufferCount;
}


#define onlyOneC 0
void ClTimer::paintGL(cl_double3 rotation, double translateZ){
	// Apply some transformations
	//glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	/*
	glRotatef(Clock.GetElapsedTime() * 50, 1.f, 0.f, 0.f);
	glRotatef(Clock.GetElapsedTime() * 30, 0.f, 1.f, 0.f);
	glRotatef(Clock.GetElapsedTime() * 90, 0.f, 0.f, 1.f);//*/

	// Draw a cube
	glColor3d(0.2,0.2,0.2);
	glBegin(GL_LINE_LOOP);
	glVertex3d(0,0,0);
	glVertex3d(boxSize.s0,0,0);
	glVertex3d(boxSize.s0,boxSize.s1,0);
	glVertex3d(0,boxSize.s1,0);
	glEnd();
	if(_3D_!=0){
		glBegin(GL_LINE_LOOP);
		glVertex3d(0,0,boxSize.s2);
		glVertex3d(boxSize.s0,0,boxSize.s2);
		glVertex3d(boxSize.s0,boxSize.s1,boxSize.s2);
		glVertex3d(0,boxSize.s1,boxSize.s2);
		glEnd();
		glBegin(GL_LINES);
		glVertex3d(0,0,0);
		glVertex3d(0,0,boxSize.s2);
		glVertex3d(boxSize.s0,0,0);
		glVertex3d(boxSize.s0,0,boxSize.s2);
		glVertex3d(boxSize.s0,boxSize.s1,0);
		glVertex3d(boxSize.s0,boxSize.s1,boxSize.s2);
		glVertex3d(0,boxSize.s1,0);
		glVertex3d(0,boxSize.s1,boxSize.s2);
		glEnd();
	}
	if(!renderBool)return;
	
	double r,x,y,z;
	
	int i,k,h;
	Circle c;
	CircleExtension* ce;
	int color = 102+(102*256)+(102*256*256);
	//err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, 0, sizeof(Circle)*readNum_render, c_CPU_render[bufferReadIndex], NULL, NULL);//&event);
	//printf("waiting for reading...\n");
	//event.wait();
	//printf("ready!\n");
	//queue.finish();
	if(useCircleExtensions){
		for(i=0; i < readNum_render; i++)
		{
			//printf("%4u: %5u\n", j, offset+i);
			c = c_CPU_render[bufferReadIndex][i];
			r = c.size;
			x = c.pos.s0;
			y = c.pos.s1;
			z = 0;
			ce = &ceBuffer[i];
			color = ce->color;
			ce->trace[ce->traceCount] = (cl_float2){x,y};
			if(connectTracePoints)
				glBegin(GL_LINE_STRIP);
			else
				glBegin(GL_POINTS);
			k = 0;
			for(h = (ce->traceFull?((ce->traceCount+1)%traceCount):0); h!=ce->traceCount; h=((h+1)%traceCount)){
				glColor4b(color/256/256,color/256%256,color%256,255*(k++)/traceCount/2);
				glVertex2f(ce->trace[h].s0, ce->trace[h].s1);
			}
			glColor4b(color/256/256,color/256%256,color%256,255*(k++)/traceCount/2);
			glVertex2f(x,y);
			ce->traceCount++;
			if(ce->traceCount>=traceCount){
				ce->traceCount = 0;
				ce->traceFull = true;
			}
			glEnd();
		}
	}
	for(i=0; i < readNum_render; i++)
	{
		//printf("%4u: %5u\n", j, offset+i);
		c = c_CPU_render[bufferReadIndex][i];
		r = c.size;
		x = c.pos.s0;
		y = c.pos.s1;
		z = 0;
		if(useCircleExtensions){
			color = ceBuffer[i].color;
		}
		//printf("Circle (r=%3f): (%4f|%4f|%4f) (%4f|%4f|%4f) (%4f|%4f|%4f)\n",r,x,y,z,c.speed.s0,c.speed.s1,0,c.force.s0,c.force.s1,0);
		if(_3D_!=0){
			//z = c_CPU_render[j][i].pos.s2;
		}
		if(_3D_==0){
			glBegin(GL_TRIANGLE_FAN);
			#if onlyOneC
				if(i==cCount-1)
					glColor3d(1,1,1); 
				else
					glColor3d(0.1, 0.1, 0.1); 
			#else
				glColor3d(1,1,1); 
			#endif
			glVertex3d(x-(r/3),y+(r/3),z);
			#if onlyOneC
				if(i==cCount-1)
					glColor3bv((byte*)&color);
					//glColor3b(color/256/256,color/256%256,color%256); 
				else
					glColor3d(0.05, 0.05, 0.05);
			#else
				glColor3bv((GLbyte*)&color);
				//glColor3b(color/256/256,color/256%256,color%256); 
			#endif
			double d = 0;
			for(int j = 0; j<=edges; j++){
				d+=step;
				//cout<<edges<<" "<<j<<endl;
				//cout<<d<<endl;
				glVertex3d(x+cos(d)*r,y+sin(d)*r,z);
			}

			glEnd();
		}
	}
	
	if(((bufferReadIndex+1)%renderBufferCount) == bufferWriteIndex){
		printf("frame buffer empty: %3d | %3d\n",bufferReadIndex,bufferWriteIndex);
	}else{
		bufferReadIndex = (bufferReadIndex+1)%renderBufferCount;
	}
	
	/*
	if(circlesBufferUsed){
		for(int i=0; i < circlesCount; i++)
		{
			//printf("%4u: %5u\n", j, offset+i);
			r = circlesBuffer[i].size;
			x = circlesBuffer[i].pos.s0;
			y = circlesBuffer[i].pos.s1;
			z = boxSize.s2;
			//printf("Circle (r=%3f): (%4.2f|%4.2f|%4.2f)\n",r,x,y,z);
			if(_3D_!=0){
				//z = circlesBuffer[i].pos.s2;
			}
			if(_3D_==0){
				glBegin(GL_TRIANGLE_FAN);
#if onlyOneC
				if(i==cCount-1)
					glColor3d(1,1,1); 
				else
					glColor3d(0.1, 0.1, 0.1); 
#else
				glColor3d(1,1,1); 
#endif
				glVertex3d(x-(r/3),y+(r/3),z);
#if onlyOneC
				if(i==cCount-1)
					glColor3d(0.4,0.4,0.4); 
				else
					glColor3d(0.05, 0.05, 0.05);
#else
				glColor3d(0.4,0.4,0.4); 
#endif
				double d = 0;
				for(int j = 0; j<=edges; j++){
					d+=step;
					//cout<<edges<<" "<<j<<endl;
					//cout<<d<<endl;
					glVertex3d(x+cos(d)*r,y+sin(d)*r,z);
				}

				glEnd();
			}
		}
	}else{
		int j = 0, k;
		int offset = 0;
		Circle c;
		CircleExtension* ce;
		int color = 102+(102*256)+(102*256*256);
		err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum), c_CPU_render[j], NULL, &event);
		//offset+=readNum;
		for(; offset<circlesCount; offset+=readNum){
			//printf("waiting for reading...\n");
			event.wait();
			//printf("ready!\n");
			if(circlesCount-(offset+readNum)>0){
				err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*(offset+readNum), sizeof(Circle)*min((circlesCount-(offset+readNum)),readNum), c_CPU_render[((j+1)%2)], NULL, &event);
			}
			//queue.finish();

			for(int i=0; i < min((circlesCount-offset),readNum); i++)
			{
				//printf("%4u: %5u\n", j, offset+i);
				c = c_CPU_render[j][i];
				r = c.size;
				x = c.pos.s0;
				y = c.pos.s1;
				z = 0;
				if(useCircleExtensions){
					ce = &ceBuffer[offset+i];
					color = ce->color;
					ce->trace[ce->traceCount] = (cl_float2){x,y};
					glBegin(GL_LINE_STRIP);
					//glBegin(GL_POINTS);
					k = 0;
					for(int h = (ce->traceFull?((ce->traceCount+1)%traceCount):0); h!=ce->traceCount; h=((h+1)%traceCount)){
						glColor4b(color/256/256,color/256%256,color%256,255*(k++)/traceCount/2);
						glVertex2f(ce->trace[h].s0, ce->trace[h].s1);
					}
					glColor4b(color/256/256,color/256%256,color%256,255*(k++)/traceCount/2);
					glVertex2f(x,y);
					ce->traceCount++;
					if(ce->traceCount>=traceCount){
						ce->traceCount = 0;
						ce->traceFull = true;
					}
					glEnd();
				}
				//printf("Circle (r=%3f): (%4f|%4f|%4f) (%4f|%4f|%4f) (%4f|%4f|%4f)\n",r,x,y,z,c.speed.s0,c.speed.s1,0,c.force.s0,c.force.s1,0);
				if(_3D_!=0){
					//z = c_CPU_render[j][i].pos.s2;
				}
				if(_3D_==0){
					glBegin(GL_TRIANGLE_FAN);
					#if onlyOneC
						if(i==cCount-1)
							glColor3d(1,1,1); 
						else
							glColor3d(0.1, 0.1, 0.1); 
					#else
						glColor3d(1,1,1); 
					#endif
					glVertex3d(x-(r/3),y+(r/3),z);
					#if onlyOneC
						if(i==cCount-1)
							glColor3bv((byte*)&color);
							//glColor3b(color/256/256,color/256%256,color%256); 
						else
							glColor3d(0.05, 0.05, 0.05);
					#else
						glColor3bv((GLbyte*)&color);
						//glColor3b(color/256/256,color/256%256,color%256); 
					#endif
					double d = 0;
					for(int j = 0; j<=edges; j++){
						d+=step;
						//cout<<edges<<" "<<j<<endl;
						//cout<<d<<endl;
						glVertex3d(x+cos(d)*r,y+sin(d)*r,z);
					}

					glEnd();
				}
			}
			j=((j+1)%2);
		}
	}*/
}

void start(ClTimer* clTimer){
	clTimer->run();
}

void ClTimer::run(){
	printf("ClTimer running!\n");
	int i = 0;
	try{
		while(true){
			err = queue.enqueueNDRangeKernel(moveStep_kernel, cl::NullRange, cl::NDRange(circlesCount), cl::NullRange, NULL, &events[eventCounter++]); 
			//printf("step %i\n", i++);
			if(eventCounter>=numEvents){
				eventCounter = 0;
				eventsFull = true;
			}
			if(eventsFull){
				//event2.at(0)=events[eventCounter];
				//printf(
				//queue.enqueueWaitForEvents(event2);
				events[eventCounter].wait();
				events[eventCounter].~Event();
				//context.release(events[eventCounter]);
				//queue.release(events[eventCounter]);
			}else{
				queue.finish();
			}
			//if(err!=CL_SUCCESS)printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
			//queue.finish();
			
			//if(i%5==0)
			if(saveBool)
			{
				//queue.finish();
				save();
			}
			//printf(".");
			frameCounter++;
			
			//*
			elapsedFrames++;
			//printf("frames: (%d/%d)=%d | %d\n", (int)fps, (int)renderFps, (int)(fps/renderFps), elapsedFrames);
			if(elapsedFrames > (int)(fps/renderFps) && glWidget != NULL){// && glWidget->drawingFinished){
				if(bufferReadIndex == ((bufferWriteIndex+1)%renderBufferCount)){
					printf("frame buffer full: %3d | %3d\n",bufferReadIndex,bufferWriteIndex);
					continue;
				}
				err = queue.enqueueReadBuffer(cl_circles, CL_FALSE, 0, sizeof(Circle)*readNum_render, c_CPU_render[bufferWriteIndex], NULL, NULL);//&event);
				bufferWriteIndex = ((bufferWriteIndex+1)%renderBufferCount);
				
				
				elapsedFrames = 0;
				/*
				glWidget->drawingFinished = false;
				//glWidget->update();
				//QCoreApplication::processEvents();
				emit glWidget->timeToRender();
				//glWidget->timeToRender();
				//glWidget->repaint();
				// */
			}// */
			
			/*
			double r,x,y,z;
			int j = 0;
			int offset = 0;
			Circle c;
			err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*offset, sizeof(Circle)*min((circlesCount-offset),readNum), c_CPU_render[j], NULL, &event);
			//offset+=readNum;
			for(; offset<circlesCount; offset+=readNum){
				event.wait();
				if(circlesCount-(offset+readNum)>0){
					err = queue.enqueueReadBuffer(cl_circles, CL_TRUE, sizeof(Circle)*(offset+readNum), sizeof(Circle)*min((circlesCount-(offset+readNum)),readNum), c_CPU_render[((j+1)%2)], NULL, &event);
				}
				//queue.finish();

				for(int i=0; i < min((circlesCount-offset),readNum); i++)
				{
					//printf("%4u: %5u\n", j, offset+i);
					c = c_CPU_render[j][i];
					r = c.size;
					x = c.pos.s0;
					y = c.pos.s1;
					z = 0;
					printf("Circle size(%3f) mass(%3f) E(%3f) ",r,c.mass,c.E);
					printf("pos(%4f|%4f|%4f) ",x,y,z);
					printf("speed(%4f|%4f|%4f) ",c.speed.s0,c.speed.s1,0.0);
					printf("force(%4f|%4f|%4f)\n",c.force.s0,c.force.s1,0.0);
				}
			}*/
		}
	}
	catch (cl::Error er) {
		printf("ERROR: %s (%d | %s)\n", er.what(), er.err(), oclErrorString(er.err()));
	}
}

char* ClTimer::file_contents(const char *filename, int *length)
{
	FILE *f = fopen(filename, "r");
	void *buffer;

	if (!f) {
		fprintf(stderr, "Unable to open %s for reading\n", filename);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	*length = ftell(f);
	fseek(f, 0, SEEK_SET);

	buffer = malloc(*length+1);
	*length = fread(buffer, 1, *length, f);
	fclose(f);
	((char*)buffer)[*length] = '\0';

	return (char*)buffer;
}

// Helper function to get error string
// *********************************************************************
const char* ClTimer::oclErrorString(cl_int error)
{
	static const char* errorString[] = {
		"CL_SUCCESS",
		"CL_DEVICE_NOT_FOUND",
		"CL_DEVICE_NOT_AVAILABLE",
		"CL_COMPILER_NOT_AVAILABLE",
		"CL_MEM_OBJECT_ALLOCATION_FAILURE",
		"CL_OUT_OF_RESOURCES",
		"CL_OUT_OF_HOST_MEMORY",
		"CL_PROFILING_INFO_NOT_AVAILABLE",
		"CL_MEM_COPY_OVERLAP",
		"CL_IMAGE_FORMAT_MISMATCH",
		"CL_IMAGE_FORMAT_NOT_SUPPORTED",
		"CL_BUILD_PROGRAM_FAILURE",
		"CL_MAP_FAILURE",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"CL_INVALID_VALUE",
		"CL_INVALID_DEVICE_TYPE",
		"CL_INVALID_PLATFORM",
		"CL_INVALID_DEVICE",
		"CL_INVALID_CONTEXT",
		"CL_INVALID_QUEUE_PROPERTIES",
		"CL_INVALID_COMMAND_QUEUE",
		"CL_INVALID_HOST_PTR",
		"CL_INVALID_MEM_OBJECT",
		"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
		"CL_INVALID_IMAGE_SIZE",
		"CL_INVALID_SAMPLER",
		"CL_INVALID_BINARY",
		"CL_INVALID_BUILD_OPTIONS",
		"CL_INVALID_PROGRAM",
		"CL_INVALID_PROGRAM_EXECUTABLE",
		"CL_INVALID_KERNEL_NAME",
		"CL_INVALID_KERNEL_DEFINITION",
		"CL_INVALID_KERNEL",
		"CL_INVALID_ARG_INDEX",
		"CL_INVALID_ARG_VALUE",
		"CL_INVALID_ARG_SIZE",
		"CL_INVALID_KERNEL_ARGS",
		"CL_INVALID_WORK_DIMENSION",
		"CL_INVALID_WORK_GROUP_SIZE",
		"CL_INVALID_WORK_ITEM_SIZE",
		"CL_INVALID_GLOBAL_OFFSET",
		"CL_INVALID_EVENT_WAIT_LIST",
		"CL_INVALID_EVENT",
		"CL_INVALID_OPERATION",
		"CL_INVALID_GL_OBJECT",
		"CL_INVALID_BUFFER_SIZE",
		"CL_INVALID_MIP_LEVEL",
		"CL_INVALID_GLOBAL_WORK_SIZE",
	};

	const int errorCount = sizeof(errorString) / sizeof(errorString[0]);

	const int index = -error;

	return (index >= 0 && index < errorCount) ? errorString[index] : "";
}
