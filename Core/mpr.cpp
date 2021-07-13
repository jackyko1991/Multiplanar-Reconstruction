#include "mpr.h"

#include <boost/filesystem.hpp>

#include <vtkvmtkITKArchetypeImageSeriesScalarReader.h>
#include <vtkvmtkCurvedMPRImageFilter2.h>

#include <observe_error.h>

#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkNIFTIImageWriter.h>
#include <vtkParametricSpline.h>
#include <vtkParametricFunctionSource.h>
#include <vtkSplineFilter.h>

#include "itkImageFileReader.h"
#include "itkImage.h"

#include <vtkXMLPolyDataWriter.h>

MPR::MPR()
{

}

MPR::~MPR()
{

}

void MPR::SetInputPath(std::string inputPath)
{
	m_inputPath = inputPath;
}

void MPR::SetCenterlinePath(std::string centerlinePath)
{
	m_centerlinePath = centerlinePath;
}

void MPR::SetOutputPath(std::string outputPath)
{
	m_outputPath = outputPath;
}

void MPR::SetInplaneSize(int inplaneSize)
{
	m_inplaneSize = inplaneSize;
}

void MPR::SetInplaneSpacing(double inplaneSpacing)
{
	m_inplaneSpacing = inplaneSpacing;
}

void MPR::SetSplineSpacing(double splineSpacing)
{
	m_splineSpacing = splineSpacing;
}

void MPR::Run()
{
	vtkSmartPointer<ErrorObserver> errorObserver = vtkSmartPointer<ErrorObserver>::New();

	// read input image
	std::cout << "Reading image..." << std::endl;
	std::cout << "Image path: " << m_inputPath << std::endl;

	std::string input_extension = boost::filesystem::extension(m_inputPath);
	if (input_extension == ".nii" || input_extension == ".NII" || input_extension == ".gz" || input_extension == ".GZ")
	{
		itk::ImageFileReader<itk::Image<float,3>>::Pointer itkReader = itk::ImageFileReader<itk::Image<float,3>>::New();
		itkReader->SetFileName(m_inputPath.c_str());
		itkReader->Update();

		//itkReader->GetOutput()->Print(std::cout);

		vtkSmartPointer<vtkvmtkITKArchetypeImageSeriesScalarReader> reader = vtkSmartPointer<vtkvmtkITKArchetypeImageSeriesScalarReader>::New();
		reader->SetArchetype(m_inputPath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		reader->SetDefaultDataSpacing(itkReader->GetOutput()->GetSpacing()[0], itkReader->GetOutput()->GetSpacing()[1], itkReader->GetOutput()->GetSpacing()[2]);
		reader->SetDefaultDataOrigin(0,0,0);
		reader->SetOutputScalarTypeToNative();
		reader->SetDesiredCoordinateOrientationToNative();
		reader->SetSingleFile(0);
		reader->Update();
		m_input->DeepCopy(reader->GetOutput());

		vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
		matrix->DeepCopy(reader->GetRasToIjkMatrix());
		m_rasToIjkMatrix->DeepCopy(matrix);

		// compute itk to vtk matrix, may need to add direction for non-orthogonal slicing images
		vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
		transform->PostMultiply();
		transform->Translate(-reader->GetOutput()->GetOrigin()[0], -reader->GetOutput()->GetOrigin()[1], -reader->GetOutput()->GetOrigin()[2]);
		// convert to LPS direction
		transform->Scale(-1* itkReader->GetOutput()->GetDirection()[0][0], -1 * itkReader->GetOutput()->GetDirection()[1][1], 1 * itkReader->GetOutput()->GetDirection()[2][2]);
		transform->Translate(m_input->GetOrigin()[0], m_input->GetOrigin()[1], m_input->GetOrigin()[2]);
		m_itkToVtkMatrix->DeepCopy(transform->GetMatrix());
	}
	else
	{
		std::cerr << "Invalid input data type, only accept NIFTI files" << std::endl;
		return;
	}

	// read centerline
	std::cout << "Reading centerline..." << std::endl;
	std::cout << "Centerline path: " << m_centerlinePath << std::endl;
	std::string centerline_extension = boost::filesystem::extension(m_centerlinePath);
	if (centerline_extension == ".vtp" || centerline_extension == ".VTP")
	{
		vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
		reader->SetFileName(m_centerlinePath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		reader->Update();
		m_centerline->DeepCopy(reader->GetOutput());
	}
	else if (centerline_extension == ".vtk" || centerline_extension == ".VTK")
	{
		vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
		reader->SetFileName(m_centerlinePath.c_str());
		reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
		m_centerline->DeepCopy(reader->GetOutput());
	}
	else
	{
		std::cerr << "Invalid centerline data type, only accept VTP or VTK files" << std::endl;
		return;
	}

	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
	transform->SetMatrix(m_itkToVtkMatrix);

	vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
	transformFilter->SetInputData(m_centerline);
	transformFilter->SetTransform(transform);
	transformFilter->Update();
	m_centerline->DeepCopy(transformFilter->GetOutput());

	// for robustness, generate a spline using centerline points
	vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
	spline->SetPoints(m_centerline->GetPoints());
	vtkSmartPointer<vtkParametricFunctionSource> functionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();
	functionSource->SetParametricFunction(spline);
	functionSource->Update();

	vtkSmartPointer<vtkPolyData> splineSource = vtkSmartPointer<vtkPolyData>::New();
	splineSource->DeepCopy(functionSource->GetOutput());
	splineSource->GetPointData()->DeepCopy(m_centerline->GetPointData());

	vtkSmartPointer<vtkSplineFilter> splineFilter = vtkSmartPointer<vtkSplineFilter>::New();
	splineFilter->SetInputData(splineSource);
	splineFilter->SetSubdivideToLength();
	splineFilter->SetLength(m_splineSpacing);
	splineFilter->Update();

	std::cout << "Performing MPR..." << std::endl;

	vtkSmartPointer<vtkvmtkCurvedMPRImageFilter2> curvedMPRImageFilter = vtkSmartPointer<vtkvmtkCurvedMPRImageFilter2>::New();
	curvedMPRImageFilter->SetInputData(m_input);
	curvedMPRImageFilter->SetParallelTransportNormalsArrayName("ParallelTransportNormals");
	curvedMPRImageFilter->SetFrenetTangentArrayName("FrenetTangent");
	curvedMPRImageFilter->SetInplaneOutputSpacing(m_inplaneSpacing, m_inplaneSpacing);
	curvedMPRImageFilter->SetInplaneOutputSize(m_inplaneSize, m_inplaneSize);
	curvedMPRImageFilter->SetReslicingBackgroundLevel(-2000);

	// iterate through all centerlineids
	curvedMPRImageFilter->SetCenterline(splineFilter->GetOutput());
	curvedMPRImageFilter->Update();
 
	vtkSmartPointer<vtkImageData> mprImage = vtkSmartPointer<vtkImageData>::New();
	mprImage->DeepCopy(curvedMPRImageFilter->GetOutput());
	mprImage->SetSpacing(m_inplaneSpacing, m_inplaneSpacing,m_splineSpacing);

	std::cout << "Write MPR file..." << std::endl;
	std::cout << "Output path: " << m_outputPath << std::endl;
	std::string output_extension = boost::filesystem::extension(m_outputPath);

	if (output_extension == ".nii" || output_extension == ".NII"  || output_extension == ".gz" || output_extension == ".GZ")
	{
		vtkSmartPointer<vtkNIFTIImageWriter> writer = vtkSmartPointer<vtkNIFTIImageWriter>::New();
		writer->SetFileName(m_outputPath.c_str());
		writer->SetInputData(mprImage);
		writer->Update();
	}
	else if (output_extension == ".vti" || output_extension == ".VTI")
	{
		vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
		writer->SetFileName(m_outputPath.c_str());
		writer->SetInputData(mprImage);
		writer->Update();
	}

	//vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
	//writer->SetFileName("Z:/projects/Carotid-Stenosis/cxx/Multiplanar-Reconstruction/Data/image.vti");
	//writer->SetInputData(m_input);
	//writer->Update();
}