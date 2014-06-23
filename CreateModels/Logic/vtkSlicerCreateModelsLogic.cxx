/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// CreateModels Logic includes
#include "vtkSlicerCreateModelsLogic.h"

// MRML includes
#include "vtkMRMLModelNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include "vtkAppendPolyData.h"
#include "vtkCubeSource.h"
#include "vtkConeSource.h"
#include "vtkCylinderSource.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerCreateModelsLogic);




void
vtkSlicerCreateModelsLogic
::CreateCylinderData( vtkPolyData* polyData, double height, double radius )
{
  vtkSmartPointer< vtkCylinderSource > s = vtkSmartPointer< vtkCylinderSource >::New();
  s->SetHeight( height );
  s->SetRadius( radius );
  s->SetResolution( 24 );
  s->Update();
  polyData->ShallowCopy(s->GetOutput());
}



void
vtkSlicerCreateModelsLogic
::CreateConeData( vtkPolyData* polyData, double height, double radius )
{
  vtkSmartPointer< vtkConeSource > s = vtkSmartPointer< vtkConeSource >::New();
  s->SetHeight( height );
  s->SetRadius( radius );
  s->SetResolution( 24 );
  s->Update();
  polyData->ShallowCopy( s->GetOutput() );
}



/**
 Creates two MRML models, one for the needle shaft and one for the centimeter marks.
 The two models can be given different colors to show the markings.
*/
void
vtkSlicerCreateModelsLogic
::CreateNeedle( double length, double radius, double tipRadius )
{
  double tip = radius * 2.0;
  
    // Create a PolyData append object to
  vtkSmartPointer< vtkAppendPolyData > appendPolyData = vtkSmartPointer< vtkAppendPolyData >::New();
  vtkSmartPointer< vtkAppendPolyData > appendShaftTip = vtkSmartPointer< vtkAppendPolyData >::New();
  vtkSmartPointer< vtkAppendPolyData > appendMarkings = vtkSmartPointer< vtkAppendPolyData >::New();
  
    // Create a cylinder to represent the needle shaft
  vtkSmartPointer< vtkPolyData > needleShaft = vtkSmartPointer< vtkPolyData >::New();
  this->CreateCylinderData( needleShaft, ( length - tip ), radius );
    
    // Set the transform of the needle shaft
  vtkSmartPointer< vtkTransform > needleShaftTransform = vtkSmartPointer< vtkTransform >::New();
  needleShaftTransform->Translate( 0, ( tip + length ) / 2.0, 0 );
    
    // Transform the needle shaft
  vtkSmartPointer< vtkTransformPolyDataFilter > needleShaftTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  needleShaftTransformFilter->SetTransform( needleShaftTransform );
  needleShaftTransformFilter->SetInputData( needleShaft );
  needleShaftTransformFilter->Update();
    	
  appendShaftTip->AddInputData( needleShaftTransformFilter->GetOutput() );
  
    // Create a cone to represent the needle tip
  vtkSmartPointer< vtkPolyData > needleTip = vtkSmartPointer< vtkPolyData >::New();
  CreateConeData( needleTip, tip, radius );
  
    // Set the transform of the needle tip
  vtkSmartPointer< vtkTransform > needleTipTransform = vtkSmartPointer< vtkTransform >::New();
  needleTipTransform->Translate( 0, tip / 2.0, 0 );
  needleTipTransform->RotateZ( -90 );
    
    // Transform the needle tip
  vtkSmartPointer< vtkTransformPolyDataFilter > needleTipTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  needleTipTransformFilter->SetTransform( needleTipTransform );
  needleTipTransformFilter->SetInputData( needleTip );
  needleTipTransformFilter->Update();
    
  appendShaftTip->AddInputData( needleTipTransformFilter->GetOutput() );
  
  // Sphere for ball tip needle
  if ( tipRadius > 0.0 )
  {
    vtkSmartPointer< vtkSphereSource > needleBallSource = vtkSmartPointer< vtkSphereSource >::New();
    needleBallSource->SetRadius( tipRadius );
    needleBallSource->SetThetaResolution( 24 );
    needleBallSource->SetPhiResolution( 24 );
    needleBallSource->Update();
    appendShaftTip->AddInputData( needleBallSource->GetOutput() );
  }
    
    
  // Add needle centimeter markings
  
  int markPositionYMm = 10; // Start at 1 cm (10 mm).
  
  double markRadiusOffset = 0.01; // So the mark is always outside the shaft.
    
  while( markPositionYMm < length )
  {
    int numberOfLines = ( markPositionYMm / 10 ) % 5; // # Check for a 5 centimeter mark
    double markerHeight = 0.3;
    if ( numberOfLines == 0 )
    {
      markerHeight = 3 * markerHeight;
      numberOfLines = 1;
    }
        
    double firstMarkPositionY = markPositionYMm - ( ( numberOfLines - 1 ) * markerHeight );
      
      // Iterate over all markings
    int line = 0;
    while( line < numberOfLines )
    {
      line = line + 1; // Increase mark count
    
      vtkSmartPointer< vtkPolyData > linePolyData = vtkSmartPointer< vtkPolyData >::New();
      this->CreateCylinderData( linePolyData, markerHeight, ( radius + markRadiusOffset ) );
        
      vtkSmartPointer< vtkTransform > lineTransform = vtkSmartPointer< vtkTransform >::New();
      lineTransform->Translate( 0, firstMarkPositionY + 2 * ( line - 1 ) * markerHeight, 0 );
        
      vtkSmartPointer< vtkTransformPolyDataFilter > lineTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
      lineTransformFilter->SetTransform( lineTransform );
      lineTransformFilter->SetInputData( linePolyData );
      lineTransformFilter->Update();
            
      appendMarkings->AddInputData( lineTransformFilter->GetOutput() );
    }
      
      // Increment the centimeter marker
    markPositionYMm = markPositionYMm + 10;
  }

  // Rotate to the proper needle orientation (X shaft)
  vtkSmartPointer< vtkTransform > xShaftTransform = vtkSmartPointer< vtkTransform >::New();
  xShaftTransform->RotateZ( -90 );

  vtkSmartPointer< vtkTransformPolyDataFilter > xShaftNeedleTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  xShaftNeedleTransformFilter->SetTransform( xShaftTransform );

  xShaftNeedleTransformFilter->SetInputData( appendShaftTip->GetOutput() );
  xShaftNeedleTransformFilter->Update();
  vtkSmartPointer< vtkPolyData > needlePolyData = xShaftNeedleTransformFilter->GetOutput();

  vtkSmartPointer< vtkTransformPolyDataFilter > xShaftMarkersTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  xShaftMarkersTransformFilter->SetTransform( xShaftTransform );

  xShaftMarkersTransformFilter->SetInputData( appendMarkings->GetOutput() );
  xShaftMarkersTransformFilter->Update();
  vtkSmartPointer< vtkPolyData > needleMarkersPolyData = xShaftMarkersTransformFilter->GetOutput();  
  
  
    // Add the needle poly data to the scene as a model
    
  vtkSmartPointer< vtkMRMLModelNode > needleModelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  needleModelNode->SetScene( this->GetMRMLScene() );
  needleModelNode->SetName( "NeedleModel" );
  needleModelNode->SetAndObservePolyData( needlePolyData );
	
  vtkSmartPointer< vtkMRMLModelDisplayNode > needleDisplayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  needleDisplayNode->SetScene( this->GetMRMLScene() );
  needleDisplayNode->SetName( "NeedleModelDisplay" );
  needleDisplayNode->SetColor( 0.0, 1.0, 1.0 );
  this->GetMRMLScene()->AddNode( needleDisplayNode );
    
  needleModelNode->SetAndObserveDisplayNodeID( needleDisplayNode->GetID() );
  needleDisplayNode->SetAmbient( 0.2 );
    
  this->GetMRMLScene()->AddNode( needleModelNode );
  
    // Add the markers model to the scene
  
  vtkSmartPointer< vtkMRMLModelNode > markersModelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  markersModelNode->SetScene( this->GetMRMLScene() );
  markersModelNode->SetName( "NeedleMarkersModel" );
  markersModelNode->SetAndObservePolyData( needleMarkersPolyData );
  
  vtkSmartPointer< vtkMRMLModelDisplayNode > markersDisplayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  markersDisplayNode->SetScene( this->GetMRMLScene() );
  markersDisplayNode->SetName( "NeedleMarkersDisplay" );
  markersDisplayNode->SetColor( 0.0, 0.0, 0.0 );
  this->GetMRMLScene()->AddNode( markersDisplayNode );
  
  markersModelNode->SetAndObserveDisplayNodeID( markersDisplayNode->GetID() );
  markersDisplayNode->SetAmbient( 0.2 );
  
  this->GetMRMLScene()->AddNode( markersModelNode );
}



void
vtkSlicerCreateModelsLogic
::CreateCube( double x, double y, double z )
{
  vtkSmartPointer< vtkCubeSource > cube = vtkSmartPointer< vtkCubeSource >::New();
  cube->SetXLength( x );
  cube->SetYLength( y );
  cube->SetZLength( z );
  cube->Update();
  
    // Add the needle poly data to the scene as a model
  
  vtkSmartPointer< vtkMRMLModelNode > modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  modelNode->SetScene( this->GetMRMLScene() );
  modelNode->SetName( "CubeModel" );
  modelNode->SetAndObservePolyData( cube->GetOutput() );

  vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  displayNode->SetScene( this->GetMRMLScene() );
  displayNode->SetName( "CubeModelDisplay" );
  this->GetMRMLScene()->AddNode( displayNode );
  
  modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  
  this->GetMRMLScene()->AddNode( modelNode );
}



void
vtkSlicerCreateModelsLogic
::CreateCylinder( double h, double r )
{
  vtkSmartPointer< vtkCylinderSource > cube = vtkSmartPointer< vtkCylinderSource >::New();
  cube->SetHeight( h );
  cube->SetRadius( r );
  cube->SetResolution( 24 );
  cube->Update();
  
    // Add the needle poly data to the scene as a model
  
  vtkSmartPointer< vtkMRMLModelNode > modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  modelNode->SetScene( this->GetMRMLScene() );
  modelNode->SetName( "CylinderModel" );
  modelNode->SetAndObservePolyData( cube->GetOutput() );

  vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  displayNode->SetScene( this->GetMRMLScene() );
  displayNode->SetName( "CylinderModelDisplay" );
  this->GetMRMLScene()->AddNode( displayNode );
  
  modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  
  this->GetMRMLScene()->AddNode( modelNode );
}



void
vtkSlicerCreateModelsLogic
::CreateSphere( double radius )
{
  vtkSmartPointer< vtkSphereSource > sphere = vtkSmartPointer< vtkSphereSource >::New();
  sphere->SetRadius( radius );
  sphere->SetThetaResolution( 24 );
  sphere->SetPhiResolution( 12 );
  
  vtkSmartPointer< vtkMRMLModelNode > modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  modelNode->SetScene( this->GetMRMLScene() );
  modelNode->SetName( "SphereModel" );
  modelNode->SetAndObservePolyData( sphere->GetOutput() );
  
  vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  displayNode->SetScene( this->GetMRMLScene() );
  displayNode->SetName( "SphereModelDisplay" );
  this->GetMRMLScene()->AddNode( displayNode );
  
  modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  this->GetMRMLScene()->AddNode( modelNode );
}



void
vtkSlicerCreateModelsLogic
::CreateCoordinate( double axisLength, double axisDiameter )
{
  
  vtkSmartPointer< vtkAppendPolyData > appendPolyData = vtkSmartPointer< vtkAppendPolyData >::New();
  
  // X axis
  
  vtkSmartPointer< vtkCylinderSource > XCylinderSource = vtkSmartPointer< vtkCylinderSource >::New();
  XCylinderSource->SetRadius( axisDiameter / 2.0 );
  XCylinderSource->SetHeight( axisLength );
  
  vtkSmartPointer< vtkTransform > XCylinderTransform = vtkSmartPointer< vtkTransform >::New();
  XCylinderTransform->RotateZ( -90.0 );
  XCylinderTransform->Translate( 0.0, axisLength / 2.0, 0.0 );
  XCylinderTransform->Update();
  
  vtkSmartPointer< vtkTransformPolyDataFilter > XCylinderTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  XCylinderTransformFilter->SetInputData( XCylinderSource->GetOutput() );
  XCylinderTransformFilter->SetTransform( XCylinderTransform );
  XCylinderTransformFilter->Update();
  
  appendPolyData->AddInputData( XCylinderTransformFilter->GetOutput() );
  
  vtkSmartPointer< vtkConeSource > XTipSource = vtkSmartPointer< vtkConeSource >::New();
  XTipSource->SetRadius( axisDiameter * 1.5 );
  XTipSource->SetHeight( axisLength / 4.0 );
  
  vtkSmartPointer< vtkTransform > XTipTransform = vtkSmartPointer< vtkTransform >::New();
  XTipTransform->Translate( axisLength + axisLength * 0.1, 0.0, 0.0 );
  XTipTransform->Update();
  
  vtkSmartPointer< vtkTransformPolyDataFilter > XTipTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  XTipTransformFilter->SetInputData( XTipSource->GetOutput() );
  XTipTransformFilter->SetTransform( XTipTransform );
  XTipTransformFilter->Update();
  
  appendPolyData->AddInputData( XTipTransformFilter->GetOutput() );
  
  
  // Y axis
  
  vtkSmartPointer< vtkCylinderSource > YCylinderSource = vtkSmartPointer< vtkCylinderSource >::New();
  YCylinderSource->SetRadius( axisDiameter / 2.0 );
  YCylinderSource->SetHeight( axisLength );
  
  vtkSmartPointer< vtkTransform > YCylinderTransform = vtkSmartPointer< vtkTransform >::New();
  YCylinderTransform->Translate( 0.0, axisLength / 2.0, 0.0 );
  YCylinderTransform->Update();
  
  vtkSmartPointer< vtkTransformPolyDataFilter > YCylinderTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  YCylinderTransformFilter->SetInputData( YCylinderSource->GetOutput() );
  YCylinderTransformFilter->SetTransform( YCylinderTransform );
  
  appendPolyData->AddInputData( YCylinderTransformFilter->GetOutput() );
  
  
  // Z axis
  
  vtkSmartPointer< vtkCylinderSource > ZCylinderSource = vtkSmartPointer< vtkCylinderSource >::New();
  ZCylinderSource->SetRadius( axisDiameter / 2.0 );
  ZCylinderSource->SetHeight( axisLength );
  
  vtkSmartPointer< vtkTransform > ZCylinderTransform = vtkSmartPointer< vtkTransform >::New();
  ZCylinderTransform->RotateX( 90.0 );
  ZCylinderTransform->Translate( 0.0, axisLength / 2.0, 0.0 );
  ZCylinderTransform->Update();
  
  vtkSmartPointer< vtkTransformPolyDataFilter > ZCylinderTransformFilter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
  ZCylinderTransformFilter->SetInputData( ZCylinderSource->GetOutput() );
  ZCylinderTransformFilter->SetTransform( ZCylinderTransform );
  
  appendPolyData->AddInputData( ZCylinderTransformFilter->GetOutput() );
  
  
  
  // Model node
  
  vtkSmartPointer< vtkMRMLModelNode > modelNode = vtkSmartPointer< vtkMRMLModelNode >::New();
  modelNode->SetScene( this->GetMRMLScene() );
  modelNode->SetName( "CoordinateModel" );
  modelNode->SetAndObservePolyData( appendPolyData->GetOutput() );
  
  vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
  displayNode->SetScene( this->GetMRMLScene() );
  displayNode->SetName( "CoordinateModelDisplay" );
  this->GetMRMLScene()->AddNode( displayNode );
  
  modelNode->SetAndObserveDisplayNodeID( displayNode->GetID() );
  
  this->GetMRMLScene()->AddNode( modelNode );
}



vtkSlicerCreateModelsLogic::vtkSlicerCreateModelsLogic()
{
  
}




vtkSlicerCreateModelsLogic::~vtkSlicerCreateModelsLogic()
{
}



//----------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}



//---------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerCreateModelsLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}