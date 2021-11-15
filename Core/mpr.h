#ifndef __MPR_H__
#define __MPR_H__

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>

class MPR
{
public:
	MPR();
	~MPR();
	void SetInputPath(std::string);
	void SetCenterlinePath(std::string);
	void SetOutputPath(std::string);
	void SetInplaneSize(int);
	void SetInplaneSpacing(double);
	void SetSplineSpacing(double);
	void SetReslicingBackgroundLevel(double);
	void Run();

private:
	int m_inplaneSize = 120;
	double m_inplaneSpacing = 0.25;
	double m_splineSpacing = 0.25;
	double m_reslicingBackgroundLevel = 0;

	std::string m_inputPath;
	std::string m_centerlinePath;
	std::string m_outputPath;
	vtkSmartPointer<vtkImageData> m_input = vtkSmartPointer<vtkImageData>::New();
	vtkSmartPointer<vtkMatrix4x4> m_rasToIjkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	vtkSmartPointer<vtkMatrix4x4> m_itkToVtkMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	vtkSmartPointer<vtkPolyData> m_centerline = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPolyData> m_output = vtkSmartPointer<vtkPolyData>::New();

};

#endif