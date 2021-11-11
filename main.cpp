#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

#include "mpr.h"

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
	po::options_description desc("Perform multiplanar reconstruction with given image and centerline.");

	int imageSize;
	double imageSpacing;
	double splineSpacing;

	desc.add_options()
		("help,h", "Display this help")
		("input,i", po::value<std::string>(), "Image File to process.")
		("centerline,c", po::value<std::string>(), "Centerline file to process.")
		("output,o", po::value<std::string>(), "Output file path.")
		("imageSize,x", po::value<int>(&imageSize)->default_value(120), "Inplane image size for MPR output (default = 120)")
		("imageSpacing,r", po::value<double>(&imageSpacing)->default_value(0.25), "Inplane image spacing for MPR output (default = 0.25)")
		("splineSpacing,s", po::value<double>(&splineSpacing)->default_value(0.25), "Subdivided centerline point spacing, equivalent to z direction resolution for MPR output (default = 0.25)")
		;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	notify(vm);

	if (vm.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("input"))
	{
		// check existence of the input file
		if (!boost::filesystem::exists(vm["input"].as<std::string>()))
		{
			std::cout << "Input file not exists" << std::endl;
			return 1;
		}
	}
	else
	{
		std::cout << "Input file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("centerline"))
	{
		// check existence of the centerline file
		if (!boost::filesystem::exists(vm["centerline"].as<std::string>()))
		{
			std::cout << "Centerline file not exists" << std::endl;
			return 1;
		}
	}
	else
	{
		std::cout << "Centerline file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	if (!vm.count("output"))
	{
		std::cout << "Output file not given" << std::endl;
		std::cout << desc << "\n";
		return 1;
	}

	MPR mpr;
	mpr.SetInputPath(vm["input"].as<std::string>());
	mpr.SetCenterlinePath(vm["centerline"].as<std::string>());
	mpr.SetOutputPath(vm["output"].as<std::string>());
	mpr.SetInplaneSize(vm["imageSize"].as<int>());
	mpr.SetInplaneSpacing(vm["imageSpacing"].as<double>());
	mpr.SetSplineSpacing(vm["splineSpacing"].as<double>());
	mpr.Run();
}