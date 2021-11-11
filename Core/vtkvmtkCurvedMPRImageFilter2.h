#ifndef vtkvmtkCurvedMPRImageFilter2_H
#define vtkvmtkCurvedMPRImageFilter2_H

#include "vtkvmtkCurvedMPRImageFilter.h"

class vtkvmtkCurvedMPRImageFilter2 : public vtkvmtkCurvedMPRImageFilter

{

public:

	static vtkvmtkCurvedMPRImageFilter2* New();

	vtkTypeMacro(vtkvmtkCurvedMPRImageFilter2, vtkvmtkCurvedMPRImageFilter);

	void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

	//@{

	/**

	* Set interpolation mode (default: nearest neighbor).

	*/

	vtkSetClampMacro(InterpolationMode, int,
		VTK_RESLICE_NEAREST, VTK_RESLICE_CUBIC);

	vtkGetMacro(InterpolationMode, int);

	void SetInterpolationModeToNearestNeighbor() {
		this->SetInterpolationMode(VTK_RESLICE_NEAREST);
	};

	void SetInterpolationModeToLinear() {
		this->SetInterpolationMode(VTK_RESLICE_LINEAR);
	};

	void SetInterpolationModeToCubic() {
		this->SetInterpolationMode(VTK_RESLICE_CUBIC);
	};



protected:
	vtkvmtkCurvedMPRImageFilter2();
	~vtkvmtkCurvedMPRImageFilter2();

	// Description:
	// This method is called by the superclass and sets the update extent of the input image to the wholeextent 
	int RequestUpdateExtent(vtkInformation *,
		vtkInformationVector **,
		vtkInformationVector *) VTK_OVERRIDE;

	// Description:
	// This method is called by the superclass and performs the actual computation of the MPR image
	virtual int RequestData(
		vtkInformation *,
		vtkInformationVector **,
		vtkInformationVector *outputVector) VTK_OVERRIDE;

	// Description:
	// This method is called by the superclass and compute the output extent, origin and spacing
	virtual int RequestInformation(vtkInformation * vtkNotUsed(request),
		vtkInformationVector **inputVector,
		vtkInformationVector *outputVector) VTK_OVERRIDE;

	//BTX

	//template<class T>

	//void FillSlice(T* outReslicePtr, T* outputImagePtr, int* resliceUpdateExtent, int* outExtent, vtkIdType* outputInc, int slice) {

	//	vtkvmtkCurvedMPRImageFilter::FillSlice<T>(outReslicePtr, outputImagePtr, resliceUpdateExtent, outExtent, outputInc, slice);

	//}

	//BTX
	template<class T>
	void FillSlice(T* outReslicePtr, T* outputImagePtr, int* resliceUpdateExtent, int* outExtent, vtkIdType* outputInc, int slice);

	int InterpolationMode;
};


#endif
