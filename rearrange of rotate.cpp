		std::cout << "90 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 90);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID+1);
		fprintf(fp2, "no used line\ndeg = %d\n", 90);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);

		std::cout << "180 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 180);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID+1);
		fprintf(fp2, "no used line\ndeg = %d\n", 180);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);

		std::cout << "270 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 270);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID+1);
		fprintf(fp2, "no used line\ndeg = %d\n", 270);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);

		/*std::cout << "360 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 360);
		fp2 = fopen(filename, "w");
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);*/

		std::cout << "360 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg-r %d.txt", mydatedir, cnt, 360);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID+1);
		fprintf(fp2, "no used line\ndeg = %d\n", 360);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);


		std::cout << "90 degree:" << endl;
		expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
		std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg-r %d.txt", mydatedir, cnt++, 90);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID+3);
		fprintf(fp2, "no used line\ndeg = %d\n", 90);

		scan_meas_cnt(pvi, fp2);
		fclose(fp2);
		std::cout << "0 degree:" << endl;
		expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
		std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg %d.txt", mydatedir, cnt, 0);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID-5);
		fprintf(fp2, "no used line\ndeg = %d\n", 0);

		scan_meas_cnt(pvi, fp2);
		fclose(fp2);
		system(filename);


		std::cout << "270 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg-r %d.txt", mydatedir, cnt, 270);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID-1);
		fprintf(fp2, "no used line\ndeg = %d\n", 270);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);

		std::cout << "180 degree:" << endl;
expecting_man=true;		WaitForSingleObject(hRotate, INFINITE);expecting_man=false;
std::cout << "measuring ..." << endl;
		sprintf(filename, "%s\\tem %d deg-r %d.txt", mydatedir, cnt, 180);
		fp2 = fopen(filename, "w");
		fprintf(fp2, "%d\n", ++dataFileID-1);
		fprintf(fp2, "no used line\ndeg = %d\n", 180);
		
		scan_meas_cnt(pvi, fp2);
		fclose(fp2);