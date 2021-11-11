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
#include <vtkCellData.h>
#include <vtkThreshold.h>
#include <vtkGeometryFilter.h>
#include <vtkUnstructuredGrid.h>

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

	vtkSmartPointer<vtkThreshold> thresholdFilter = vtkSmartPointer<vtkThreshold>::New();
	thresholdFilter->SetInputData(m_centerline);
	thresholdFilter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "CenterlineIds");

	// loop through centerlineIds
	for (int i = m_centerline->GetCellData()->GetArray("CenterlineIds")->GetRange()[0]; i < m_centerline->GetCellData()->GetArray("CenterlineIds")->GetRange()[1]+1; i++)
	{
		thresholdFilter->ThresholdBetween(i, i);
		thresholdFilter->Update();

		vtkSmartPointer<vtkGeometryFilter> geomFilter = vtkSmartPointer<vtkGeometryFilter>::New();
		geomFilter->SetInputData(thresholdFilter->GetOutput());
		geomFilter->Update();

		if (geomFilter->GetOutput()->GetNumberOfPoints() == 0)
			continue;

		// for robustness, generate a spline using centerline points
		vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
		spline->SetPoints(geomFilter->GetOutput()->GetPoints());
		vtkSmartPointer<vtkParametricFunctionSource> functionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();
		functionSource->SetParametricFunction(spline);
		functionSource->Update();

		vtkSmartPointer<vtkPolyData> splineSource = vtkSmartPointer<vtkPolyData>::New();
		splineSource->DeepCopy(functionSource->GetOutput());
		splineSource->GetPointData()->DeepCopy(geomFilter->GetOutput()->GetPointData());

		vtkSmartPointer<vtkSplineFilter> splineFilter = vtkSmartPointer<vtkSplineFilter>::New();
		splineFilter->SetInputData(splineSource);
		splineFilter->SetSubdivideToLength();
		splineFilter->SetLength(m_splineSpacing);
		splineFilter->Update();

		std::cout << "Performing MPR for branch " << i <<"..." << std::endl;

		////save transformed centerline
		//vtkSmartPointer<vtkXMLPolyDataWriter> writer1 = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
		//writer1->SetFileName("Z:/projects/Carotid-Stenosis/cxx/Multiplanar-Reconstruction/Data/centerline_tfm.vtp");
		//writer1->SetInputData(splineFilter->GetOutput());
		//writer1->Update();

		//vtkSmartPointer<vtkXMLImageDataWriter> writer2 = vtkSmartPointer<vtkXMLImageDataWriter>::New();
		//writer2->SetFileName("Z:/projects/Carotid-Stenosis/cxx/Multiplanar-Reconstruction/Data/image.vti");
		//writer2->SetInputData(m_input);
		//writer2->Update();

		//std::cout << "inplane spacing: " << m_inplaneSpacing << std::endl;
		//std::cout << "inplane size: " << m_inplaneSize << std::endl;
		//std::cout << "spline spacing: " << m_splineSpacing << std::endl;

		vtkSmartPointer<vtkvmtkCurvedMPRImageFilter2> curvedMPRImageFilter = vtkSmartPointer<vtkvmtkCurvedMPRImageFilter2>::New();
		curvedMPRImageFilter->SetInputData(m_input);
		curvedMPRImageFilter->SetParallelTransportNormalsArrayName("ParallelTransportNormals");
		curvedMPRImageFilter->SetFrenetTangentArrayName("FrenetTangent");
		curvedMPRImageFilter->SetInplaneOutputSpacing(m_inplaneSpacing, m_inplaneSpacing);
		curvedMPRImageFilter->SetInplaneOutputSize(m_inplaneSize, m_inplaneSize);
		curvedMPRImageFilter->SetReslicingBackgroundLevel(-2000);

		// iterate through all centerlineids
		curvedMPRImageFilter->SetCenterline(splineFilter->GetOutput());
		//curvedMPRImageFilter->SetCenterline(m_centerline);
		curvedMPRImageFilter->Update();
		//curvedMPRImageFilter->GetCenterline()->Print(std::cout);
		//curvedMPRImageFilter->GetOutput()->Print(std::cout);

		vtkSmartPointer<vtkImageData> mprImage = vtkSmartPointer<vtkImageData>::New();
		mprImage->DeepCopy(curvedMPRImageFilter->GetOutput());
		mprImage->SetSpacing(m_inplaneSpacing, m_inplaneSpacing, m_splineSpacing);

		std::cout << "Write MPR file..." << std::endl;
		boost::filesystem::path outputPath(m_outputPath);
		std::string output_extension = boost::filesystem::extension(m_outputPath);
		std::string output_dir = outputPath.parent_path().string();
		std::string id_outputPath;

		if (output_extension == ".gz" || output_extension == ".GZ")
		{
			std::string output_prefix = outputPath.stem().stem().string();
			id_outputPath = output_dir + "/" + output_prefix + "_" + std::to_string(i) + ".nii.gz";

		}
		else
		{
			std::string output_prefix = outputPath.stem().string();
			id_outputPath = output_dir + "/" + output_prefix + "_" + std::to_string(i) + output_extension;
		}

		std::cout << "Output path: " << id_outputPath << std::endl;

		if (output_extension == ".nii" || output_extension == ".NII" || output_extension == ".gz" || output_extension == ".GZ")
		{
			vtkSmartPointer<vtkNIFTIImageWriter> writer = vtkSmartPointer<vtkNIFTIImageWriter>::New();
			writer->SetFileName(id_outputPath.c_str());
			writer->SetInputData(mprImage);
			writer->Update();
		}
		else if (output_extension == ".vti" || output_extension == ".VTI")
		{
			vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
			writer->SetFileName(id_outputPath.c_str());
			writer->SetInputData(mprImage);
			writer->Update();
		}
	}

	//vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
	//writer->SetFileName("Z:/projects/Carotid-Stenosis/cxx/Multiplanar-Reconstruction/Data/image.vti");
	//writer->SetInputData(m_input);
	//writer->Update();
}