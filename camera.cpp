#include "camera.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

Camera::Camera()
{
	m_fps = 30;
}

bool Camera::open(std::string filename)
{
	m_fileName = filename;
	
	// Convert filename to number if you want to
	// open webcam stream
	std::istringstream iss(filename.c_str());
	int devid;
	bool isOpen;
	if(!(iss >> devid))
	{
		isOpen = m_cap.open(filename.c_str());
	}
	else 
	{
		isOpen = m_cap.open(devid);
	}
	
	if(!isOpen)
	{
		std::cerr << "Unable to open video file." << std::endl;
		return false;
	}
	
	// set framerate, if unable to read framerate, set it to 30
	m_fps = m_cap.get(CV_CAP_PROP_FPS);
	if(m_fps == 0)
		m_fps = 30;
}

void Camera::play()
{
	// Create main window
	//namedWindow("Video", CV_WINDOW_AUTOSIZE);
	bool isReading = true;
	// Compute time to wait to obtain wanted framerate
	int timeToWait = 1000/m_fps;
	
	//nos variables
	int threshold1=100;
	int threshold2=500;
	Mat background, l_frame, frame2, dst, b_frame, drawing, kernel;
	vector<Vec4i> lines;
	int nbGauche=0,nbDroite=0;
	
	//lecture première image
	isReading = m_cap.read(m_frame);
	isReading = m_cap.read(background);
	
	while(isReading) //tant que la video continue
	{
		// Get frame from stream
		isReading = m_cap.read(m_frame);
		CvSize dim= m_frame.size();
		int h=dim.height;
		int w=dim.width;
		
		if(isReading)
		{
			//Show frame in main window
			//imshow("Video",m_frame);
			m_frame.copyTo(l_frame); //creation d'une copie de l_frame

			///Detection des bordures de la route
			if(lines.size()==0){
				Canny(m_frame, dst, threshold1, threshold2, 3); //detecteur de Canny
				HoughLinesP(dst, lines, 1, CV_PI/180, 40, 100, 10 ); //creation des lignes de Hough
			}
			imshow("Canny", dst);

			for( size_t i = 0; i < lines.size(); i++ ) //on parcourt toutes les lignes de l'image pour pouvoir allonger les lignes deja creees
			{					
					///allongement des lignes sur toute la longueur de l'image dans la même direction que la ligne de base
					Vec4i l = lines[i];
					double a=0.0,b=0.0;
					int x1,x2;
					if (l[0]-l[2]!=0){
						a= (double)((l[1]-l[3]))/(double)((l[0]-l[2]));	
						if (a!=0.0){
							b= (double)l[1]-a*(double)l[0];	
							x1=(-b/a);
							x2=(h-b)/a;
						}
						else{ //l1-l3=0 point horizontaux									
							b= (double)l[1];
							x1=0;
							x2=w;
						}
					}
					else{ //verticaux
						x1=l[0];
						x2=l[0];
					}						
					
					line(l_frame, Point(x1, 0), Point(x2,h), Scalar(0,0,255), 3, CV_AA); //On actualise les lignes sur l_frame
							
			}
			
			//imshow("Hough",dst);
					
			//on soustraie le fond à l'image
			absdiff(m_frame, background, frame2);
			//imshow("soustraction",frame2);
			
			//on binarise et on seuil
			cvtColor(frame2, frame2, CV_BGR2GRAY);
			// Set threshold and maxValue
			double thresh = 50;
			double maxValue = 255; 			 
			// Binary Threshold
			threshold(frame2,b_frame, thresh, maxValue, THRESH_BINARY);
			//imshow("Image binarisee",b_frame);
			
			//on effectue une fermeture			
			dilate(b_frame, b_frame, Mat(),Point(-1,-1),4,1,1);
			erode(b_frame, b_frame, Mat(),Point(-1,-1),4,1,1);
			//imshow("Ouvert",b_frame);
			
			///Detection des formes
			vector<vector<Point> > contours;
			vector<Vec4i> hierarchy;
			findContours(b_frame, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); //fonction qui permet de trouver les contours de l'image
			vector<vector<Point> > contours_poly( contours.size() );
			vector<Rect> boundRect( contours.size() );
			Mat drawing = Mat::zeros( b_frame.size(), CV_8UC3 );
			Scalar color = Scalar( rand()&255, rand()&255, rand()&255 );
			Rect rectangleG=Rect(0,100,w/2,8); //creation du rectangle de la voie de gauche
			rectangle( l_frame, rectangleG.tl(), rectangleG.br(), CV_RGB(0,0,255), 2, 8, 0 ); //on affiche la zone étudiée dans un zone rectangle bleu
			Rect rectangleD=Rect(w/2,170,w/2,8); //creation du rectangle de la voie de droite
			rectangle( l_frame, rectangleD.tl(), rectangleD.br(), CV_RGB(0,0,255), 2, 8, 0 ); //on affiche la zone étudiée dans un zone rectangle bleu
		
			for( int i = 0; i< contours.size(); i++ )
				 {					
					drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() ); //On crée les contours dans drawing
					approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
					boundRect[i] = boundingRect(Mat(contours_poly[i])); //creation des rectangles autour de chaque voiture
					//circle(l_frame,Point(boundRect[i].x+boundRect[i].width/2,boundRect[i].y+boundRect[i].height/2),2,CV_RGB(0,255,0),1,8,0); //affiche le barycentre des contours rectangulaires
					
					///Comptage des voitures
					//on teste si une voiture (le point du rectangle qui l'entoure) passe dans le rectangle de la voie de gauche
					if (rectangleG.contains(Point(boundRect[i].x,boundRect[i].y)))
					{
						nbGauche++; //Si c'est la cas on incremente, il y a une voiture de plus
					}
					//on teste si une voiture (le point du rectangle qui l'entoure) passe dans le rectangle de la voie de droite
					if (rectangleD.contains(Point(boundRect[i].x,boundRect[i].y)))
					{
						nbDroite++; //Si c'est la cas on incremente, il y a une voiture de plus
					}
					rectangle( l_frame, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 ); //on ajoute les rectangles a l_frame
				 }
				 
			//imshow("Formes", drawing);
			
			string gauche = static_cast<ostringstream*>( &(ostringstream() << nbGauche) )->str(); //conversion int to string
			string droite = static_cast<ostringstream*>( &(ostringstream() << nbDroite) )->str(); //conversion int to string

			//on ecrit sur l'image le nombre de voiture sur la voie de gauche en temps réel
			putText(l_frame,"Nombre de voitures voie de gauche : ",Point(230,190),FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255),1.5);
			putText(l_frame,gauche,Point(540,190),FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255),1.5);
			
			//on ecrit sur l'image le nombre de voiture sur la voie de droite en temps réel
			putText(l_frame,"Nombre de voitures voie de droite : ",Point(230,210),FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255),1.5);
			putText(l_frame,droite,Point(540,210),FONT_HERSHEY_SIMPLEX,0.5,CV_RGB(255,255,255),1.5);
			
			int nbTotal = nbGauche+nbDroite; //nombre total de voitures
			string total = static_cast<ostringstream*>( &(ostringstream() << nbTotal) )->str(); //conversion int to string
			
			//on ecrit sur l'image le nombre de voiture total en temps réel
			putText(l_frame,"Total : ",Point(350,280),FONT_HERSHEY_SIMPLEX,0.5,color,1.5);
			putText(l_frame,total,Point(410,280),FONT_HERSHEY_SIMPLEX,0.5,color,1.5);
			
			imshow("Final video",l_frame); //on affiche l_frame --> l'image finale 
						
			/// --------------------------------		
				
		}
		else
		{
			std::cerr << "Unable to read device" << std::endl;
		}
		
		// If escape key is pressed, quit the program
		if(waitKey(timeToWait)%256 == 27)
		{
			std::cerr << "Stopped by user" << std::endl;
			isReading = false;
		}	
	}	
}

bool Camera::close()
{
	// Close the stream
	m_cap.release();
	
	// Close all the windows
	destroyAllWindows();
	usleep(100000);
}


























