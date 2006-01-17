/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkGrayscaleMorphologicalClosingImageFilter.txx,v $
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkGrayscaleMorphologicalClosingImageFilter_txx
#define __itkGrayscaleMorphologicalClosingImageFilter_txx

#include "itkGrayscaleMorphologicalClosingImageFilter.h"
#include "itkGrayscaleErodeImageFilter.h"
#include "itkGrayscaleDilateImageFilter.h"
#include "itkProgressAccumulator.h"
#include "itkCropImageFilter.h"
#include "itkConstantPadImageFilter.h"

namespace itk {

template<class TInputImage, class TOutputImage, class TKernel>
GrayscaleMorphologicalClosingImageFilter<TInputImage, TOutputImage, TKernel>
::GrayscaleMorphologicalClosingImageFilter()
  : m_Kernel()
{
  m_SafeBorder = true;
}

template <class TInputImage, class TOutputImage, class TKernel>
void 
GrayscaleMorphologicalClosingImageFilter<TInputImage, TOutputImage, TKernel>
::GenerateInputRequestedRegion()
{
  // call the superclass' implementation of this method
  Superclass::GenerateInputRequestedRegion();

  // get pointers to the input and output
  typename Superclass::InputImagePointer  inputPtr =
    const_cast< TInputImage * >( this->GetInput() );

  if ( !inputPtr )
    {
    return;
    }

  // get a copy of the input requested region (should equal the output
  // requested region)
  typename TInputImage::RegionType inputRequestedRegion;
  inputRequestedRegion = inputPtr->GetRequestedRegion();

  // pad the input requested region by the operator radius
  inputRequestedRegion.PadByRadius( m_Kernel.GetRadius() );

  // crop the input requested region at the input's largest possible region
  if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
    {
    inputPtr->SetRequestedRegion( inputRequestedRegion );
    return;
    }
  else
    {
    // Couldn't crop the region (requested region is outside the largest
    // possible region).  Throw an exception.

    // store what we tried to request (prior to trying to crop)
    inputPtr->SetRequestedRegion( inputRequestedRegion );

    // build an exception
    InvalidRequestedRegionError e(__FILE__, __LINE__);
    OStringStream msg;
    msg << static_cast<const char *>(this->GetNameOfClass())
        << "::GenerateInputRequestedRegion()";
    e.SetLocation(msg.str().c_str());
    e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
    e.SetDataObject(inputPtr);
    throw e;
    }
}


template<class TInputImage, class TOutputImage, class TKernel>
void
GrayscaleMorphologicalClosingImageFilter<TInputImage, TOutputImage, TKernel>
::GenerateData()
{
  // Allocate the outputs
  this->AllocateOutputs();

  /** set up erosion and dilation methods */
  typename GrayscaleDilateImageFilter<TInputImage, TInputImage, TKernel>::Pointer
    dilate = GrayscaleDilateImageFilter<TInputImage, TInputImage, TKernel>::New();

  typename GrayscaleErodeImageFilter<TInputImage, TOutputImage, TKernel>::Pointer
    erode = GrayscaleErodeImageFilter<TInputImage, TOutputImage, TKernel>::New();

  // create the pipeline without input and output image
  dilate->ReleaseDataFlagOn();
  dilate->SetKernel( this->GetKernel() );

  erode->SetKernel( this->GetKernel() );
  erode->ReleaseDataFlagOn();
  erode->SetInput( dilate->GetOutput() );

  // now we have 2 cases:
  // + SafeBorder is true so we need to create a bigger image use it as input
  //   and crop the image to the normal output image size
  // + SafeBorder is false; we just have to connect filters
  if ( m_SafeBorder )
    {
    typedef typename itk::ConstantPadImageFilter<InputImageType, InputImageType> PadType;
    typename PadType::Pointer pad = PadType::New();
    pad->SetPadLowerBound( m_Kernel.GetRadius().m_Size );
    pad->SetPadUpperBound( m_Kernel.GetRadius().m_Size );
    pad->SetConstant( NumericTraits<typename InputImageType::PixelType>::max() );
    pad->SetInput( this->GetInput() );

    dilate->SetInput( pad->GetOutput() );
    
    typedef typename itk::CropImageFilter<TOutputImage, TOutputImage> CropType;
    typename CropType::Pointer crop = CropType::New();
    crop->SetInput( erode->GetOutput() );
    crop->SetUpperBoundaryCropSize( m_Kernel.GetRadius() );
    crop->SetLowerBoundaryCropSize( m_Kernel.GetRadius() );
    
    ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
    progress->SetMiniPipelineFilter(this);
    progress->RegisterInternalFilter(pad, .1f);
    progress->RegisterInternalFilter(erode, .4f);
    progress->RegisterInternalFilter(dilate, .4f);
    progress->RegisterInternalFilter(crop, .1f);
    
    crop->GraftOutput( this->GetOutput() );
    /** execute the minipipeline */
    crop->Update();
  
    /** graft the minipipeline output back into this filter's output */
    this->GraftOutput( crop->GetOutput() );
    }
  else
    {
    /** set up the minipipeline */
    ProgressAccumulator::Pointer progress = ProgressAccumulator::New();
    progress->SetMiniPipelineFilter(this);
    progress->RegisterInternalFilter(erode, .5f);
    progress->RegisterInternalFilter(dilate, .5f);
    
    dilate->SetInput( this->GetInput() );
    erode->GraftOutput( this->GetOutput() );
  
    /** execute the minipipeline */
    erode->Update();
  
    /** graft the minipipeline output back into this filter's output */
    this->GraftOutput( erode->GetOutput() );
    }
}

template<class TInputImage, class TOutputImage, class TKernel>
void
GrayscaleMorphologicalClosingImageFilter<TInputImage, TOutputImage, TKernel>
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Kernel: " << m_Kernel << std::endl;
  os << indent << "SafeBorder: " << m_SafeBorder << std::endl;
}

}// end namespace itk
#endif
