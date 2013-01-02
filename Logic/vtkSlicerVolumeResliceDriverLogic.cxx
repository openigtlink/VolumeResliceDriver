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

// VolumeResliceDriver includes
#include "vtkSlicerVolumeResliceDriverLogic.h"

// MRML includes
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSliceNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>

// STD includes
#include <cassert>



vtkStandardNewMacro(vtkSlicerVolumeResliceDriverLogic);



vtkSlicerVolumeResliceDriverLogic
::vtkSlicerVolumeResliceDriverLogic()
{
}



vtkSlicerVolumeResliceDriverLogic
::~vtkSlicerVolumeResliceDriverLogic()
{
  this->ClearObservedNodes();
}



void vtkSlicerVolumeResliceDriverLogic
::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Number of observed nodes: " << this->ObservedNodes.size() << std::endl;
  os << indent << "Observed nodes:";
  
  for ( unsigned int i = 0; i < this->ObservedNodes.size(); ++ i )
  {
    os << " " << this->ObservedNodes[ i ]->GetID();
  }
  
  os << std::endl;
}



void vtkSlicerVolumeResliceDriverLogic
::SetDriverForSlice( std::string nodeID, vtkMRMLSliceNode* sliceNode )
{
  vtkMRMLNode* node = this->GetMRMLScene()->GetNodeByID( nodeID );
  if ( node == NULL )
  {
    sliceNode->RemoveAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE );
    return;
  }
  
  vtkMRMLTransformableNode* tnode = vtkMRMLTransformableNode::SafeDownCast( node );
  if ( tnode == NULL )
  {
    sliceNode->RemoveAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE );
    return;
  }
  
  sliceNode->SetAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE, nodeID.c_str() );
  this->AddObservedNode( tnode );
  
  this->UpdateSliceIfObserved( sliceNode );
}



void vtkSlicerVolumeResliceDriverLogic
::SetMethodForSlice( int method, vtkMRMLSliceNode* sliceNode )
{
  if ( sliceNode == NULL )
  {
    return;
  }
  
  if ( method == this->METHOD_DEFAULT )
  {
    method = this->METHOD_POSITION;
  }
  
  std::stringstream methodSS;
  methodSS << method;
  sliceNode->SetAttribute( VOLUMERESLICEDRIVER_METHOD_ATTRIBUTE, methodSS.str().c_str() );
  
  this->UpdateSliceIfObserved( sliceNode );
}



void vtkSlicerVolumeResliceDriverLogic
::SetOrientationForSlice( int orientation, vtkMRMLSliceNode* sliceNode )
{
  if ( sliceNode == NULL )
  {
    return;
  }
  
  if ( orientation == this->ORIENTATION_DEFAULT )
  {
    orientation = this->ORIENTATION_INPLANE;
  }
  
  std::stringstream orientationSS;
  orientationSS << orientation;
  sliceNode->SetAttribute( VOLUMERESLICEDRIVER_ORIENTATION_ATTRIBUTE, orientationSS.str().c_str() );
  
  this->UpdateSliceIfObserved( sliceNode );
}



void vtkSlicerVolumeResliceDriverLogic
::AddObservedNode( vtkMRMLTransformableNode* node )
{
  for ( unsigned int i = 0; i < this->ObservedNodes.size(); ++ i )
  {
    if ( node == this->ObservedNodes[ i ] )
    {
      return;
    }
  }
  
  int wasModifying = this->StartModify();
  
  vtkMRMLTransformableNode* newNode = NULL;
  
  vtkSmartPointer< vtkIntArray > events = vtkSmartPointer< vtkIntArray >::New();
  events->InsertNextValue( vtkMRMLTransformableNode::TransformModifiedEvent );
  events->InsertNextValue( vtkCommand::ModifiedEvent );
  vtkSetAndObserveMRMLNodeEventsMacro( newNode, node, events );
  this->ObservedNodes.push_back( newNode );
  
  this->EndModify( wasModifying );
}



void vtkSlicerVolumeResliceDriverLogic
::ClearObservedNodes()
{
  for ( unsigned int i = 0; i < this->ObservedNodes.size(); ++ i )
  {
    // this->ObservedNodes[ 1 ]->RemoveAllObservers();
    vtkSetAndObserveMRMLNodeMacro( this->ObservedNodes[ i ], 0 );
  }
  
  this->ObservedNodes.clear();
}



void vtkSlicerVolumeResliceDriverLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerVolumeResliceDriverLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}



/// This method is called by Slicer after each significant MRML scene event (import, load, etc.)
void vtkSlicerVolumeResliceDriverLogic
::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
  
  // Check if any of the slice nodes contain driver transoforms that need to be observed.
  
  vtkCollection* sliceNodes = this->GetMRMLScene()->GetNodesByClass( "vtkMRMLSliceNode" );
  vtkCollectionIterator* sliceIt = vtkCollectionIterator::New();
  sliceIt->SetCollection( sliceNodes );
  for ( sliceIt->InitTraversal(); ! sliceIt->IsDoneWithTraversal(); sliceIt->GoToNextItem() )
  {
    vtkMRMLSliceNode* slice = vtkMRMLSliceNode::SafeDownCast( sliceIt->GetCurrentObject() );
    if ( slice == NULL )
    {
      continue;
    }
    const char* driverCC = slice->GetAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE );
    if ( driverCC == NULL )
    {
      continue;
    }
    vtkMRMLNode* driverNode = this->GetMRMLScene()->GetNodeByID( driverCC );
    if ( driverNode == NULL )
    {
      continue;
    }
    vtkMRMLTransformableNode* driverTransformable = vtkMRMLTransformableNode::SafeDownCast( driverNode );
    if ( driverTransformable == NULL )
    {
      continue;
    }
    this->AddObservedNode( driverTransformable );
  }
  sliceIt->Delete();
  sliceNodes->Delete();
  
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSlicerVolumeResliceDriverLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerVolumeResliceDriverLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}



void vtkSlicerVolumeResliceDriverLogic
::OnMRMLNodeModified( vtkMRMLNode* node )
{
  std::cout << "Observed node modified." << std::endl;
}



void vtkSlicerVolumeResliceDriverLogic
::ProcessMRMLNodesEvents( vtkObject* caller, unsigned long event, void * callData )
{
  
  if ( event != vtkMRMLTransformableNode::TransformModifiedEvent )
  {
    this->Superclass::ProcessMRMLNodesEvents( caller, event, callData );
    return;
  }
  
  vtkMRMLTransformableNode* callerNode = vtkMRMLTransformableNode::SafeDownCast( caller );
  if ( callerNode == NULL )
  {
    return;
  }
  std::string callerNodeID( callerNode->GetID() );
  
  vtkMRMLNode* node = 0;
  std::vector< vtkMRMLSliceNode* > SlicesToDrive;
  
  vtkCollection* sliceNodes = this->GetMRMLScene()->GetNodesByClass( "vtkMRMLSliceNode" );
  vtkCollectionIterator* sliceIt = vtkCollectionIterator::New();
  sliceIt->SetCollection( sliceNodes );
  sliceIt->InitTraversal();
  for ( unsigned int i = 0; i < sliceNodes->GetNumberOfItems(); ++ i )
  {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast( sliceIt->GetCurrentObject() );
    sliceIt->GoToNextItem();
    const char* driverCC = sliceNode->GetAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE );
    if (    sliceNode != NULL
         && driverCC != NULL
         && callerNodeID.compare( std::string( driverCC ) ) == 0 )
    {
      SlicesToDrive.push_back( sliceNode );
    }
  }
  sliceIt->Delete();
  sliceNodes->Delete();
  
  for ( unsigned int i = 0; i < SlicesToDrive.size(); ++ i )
  {
    this->UpdateSliceByTransformableNode( callerNode, SlicesToDrive[ i ] );
  }
}



void vtkSlicerVolumeResliceDriverLogic
::UpdateSliceByTransformableNode( vtkMRMLTransformableNode* tnode, vtkMRMLSliceNode* sliceNode )
{
  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast( tnode );
  if ( transformNode != NULL )
  {
    this->UpdateSliceByTransformNode( transformNode, sliceNode );
  }
  
  vtkMRMLScalarVolumeNode* imageNode = vtkMRMLScalarVolumeNode::SafeDownCast( tnode );
  if ( imageNode != NULL )
  {
    this->UpdateSliceByImageNode( imageNode, sliceNode );
  }
}



void vtkSlicerVolumeResliceDriverLogic
::UpdateSliceByTransformNode( vtkMRMLLinearTransformNode* tnode, vtkMRMLSliceNode* sliceNode )
{
  if ( ! tnode)
  {
    return;
  }

  vtkSmartPointer< vtkMatrix4x4 > transform = vtkSmartPointer< vtkMatrix4x4 >::New();
  transform->Identity();
  int getTransf = tnode->GetMatrixTransformToWorld( transform );
  if( getTransf != 0 )
  {
    this->UpdateSlice( transform, sliceNode );
  }
}


void vtkSlicerVolumeResliceDriverLogic
::UpdateSliceByImageNode( vtkMRMLScalarVolumeNode* inode, vtkMRMLSliceNode* sliceNode )
{
  vtkMRMLVolumeNode* volumeNode = inode;

  if (volumeNode == NULL)
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> rtimgTransform = vtkSmartPointer<vtkMatrix4x4>::New();
  volumeNode->GetIJKToRASMatrix(rtimgTransform);

  float tx = rtimgTransform->GetElement(0, 0);
  float ty = rtimgTransform->GetElement(1, 0);
  float tz = rtimgTransform->GetElement(2, 0);
  float sx = rtimgTransform->GetElement(0, 1);
  float sy = rtimgTransform->GetElement(1, 1);
  float sz = rtimgTransform->GetElement(2, 1);
  float nx = rtimgTransform->GetElement(0, 2);
  float ny = rtimgTransform->GetElement(1, 2);
  float nz = rtimgTransform->GetElement(2, 2);
  float px = rtimgTransform->GetElement(0, 3);
  float py = rtimgTransform->GetElement(1, 3);
  float pz = rtimgTransform->GetElement(2, 3);

  vtkImageData* imageData;
  imageData = volumeNode->GetImageData();
  int size[3];
  imageData->GetDimensions(size);

  // normalize
  float psi = sqrt(tx*tx + ty*ty + tz*tz);
  float psj = sqrt(sx*sx + sy*sy + sz*sz);
  float psk = sqrt(nx*nx + ny*ny + nz*nz);
  float ntx = tx / psi;
  float nty = ty / psi;
  float ntz = tz / psi;
  float nsx = sx / psj;
  float nsy = sy / psj;
  float nsz = sz / psj;
  float nnx = nx / psk;
  float nny = ny / psk;
  float nnz = nz / psk;

  // Shift the center
  // NOTE: The center of the image should be shifted due to different
  // definitions of image origin between VTK (Slicer) and OpenIGTLink;
  // OpenIGTLink image has its origin at the center, while VTK image
  // has one at the corner.

  float hfovi = psi * size[0] / 2.0;
  float hfovj = psj * size[1] / 2.0;
  //float hfovk = psk * imgheader->size[2] / 2.0;
  float hfovk = 0;

  float cx = ntx * hfovi + nsx * hfovj + nnx * hfovk;
  float cy = nty * hfovi + nsy * hfovj + nny * hfovk;
  float cz = ntz * hfovi + nsz * hfovj + nnz * hfovk;

  rtimgTransform->SetElement(0, 0, ntx);
  rtimgTransform->SetElement(1, 0, nty);
  rtimgTransform->SetElement(2, 0, ntz);
  rtimgTransform->SetElement(0, 1, nsx);
  rtimgTransform->SetElement(1, 1, nsy);
  rtimgTransform->SetElement(2, 1, nsz);
  rtimgTransform->SetElement(0, 2, nnx);
  rtimgTransform->SetElement(1, 2, nny);
  rtimgTransform->SetElement(2, 2, nnz);
  rtimgTransform->SetElement(0, 3, px + cx);
  rtimgTransform->SetElement(1, 3, py + cy);
  rtimgTransform->SetElement(2, 3, pz + cz);

  vtkMRMLLinearTransformNode* parentNode =
    vtkMRMLLinearTransformNode::SafeDownCast(volumeNode->GetParentTransformNode());
  if (parentNode)
    {
    vtkSmartPointer<vtkMatrix4x4> parentTransform = vtkSmartPointer<vtkMatrix4x4>::New();
    parentTransform->Identity();
    int r = parentNode->GetMatrixTransformToWorld(parentTransform);
    if (r)
      {
      vtkSmartPointer<vtkMatrix4x4> transform = vtkSmartPointer<vtkMatrix4x4>::New();
      vtkMatrix4x4::Multiply4x4(parentTransform, rtimgTransform,  transform);
      this->UpdateSlice( transform, sliceNode );
      return;
      }
    }

  this->UpdateSlice( rtimgTransform, sliceNode );

}


void vtkSlicerVolumeResliceDriverLogic
::UpdateSlice( vtkMatrix4x4* transform, vtkMRMLSliceNode* sliceNode )
{
  int method = METHOD_POSITION;
  const char* methodCC = sliceNode->GetAttribute( VOLUMERESLICEDRIVER_METHOD_ATTRIBUTE );
  if ( methodCC != NULL )
  {
    std::stringstream methodSS( methodCC );
    methodSS >> method;
  }
  
  int orientation = ORIENTATION_INPLANE;
  const char* orientationCC = sliceNode->GetAttribute( VOLUMERESLICEDRIVER_ORIENTATION_ATTRIBUTE );
  if ( orientationCC != NULL )
  {
    std::stringstream orientationSS( orientationCC );
    orientationSS >> orientation;
  }
  
  float tx = transform->Element[0][0];
  float ty = transform->Element[1][0];
  float tz = transform->Element[2][0];
  float nx = transform->Element[0][2];
  float ny = transform->Element[1][2];
  float nz = transform->Element[2][2];
  float px = transform->Element[0][3];
  float py = transform->Element[1][3];
  float pz = transform->Element[2][3];

  if ( orientation == vtkSlicerVolumeResliceDriverLogic::ORIENTATION_INPLANE)
    {
    if ( method == vtkSlicerVolumeResliceDriverLogic::METHOD_ORIENTATION)
      {
      sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 1);
      }
    else
      {
      sliceNode->SetOrientationToAxial();
      sliceNode->JumpSlice(px, py, pz);
      }
    }
  else if ( orientation == vtkSlicerVolumeResliceDriverLogic::ORIENTATION_INPLANE90 )
    {
    if ( method == vtkSlicerVolumeResliceDriverLogic::METHOD_ORIENTATION )
      {
      sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 2);
      }
    else
      {
      sliceNode->SetOrientationToSagittal();
      sliceNode->JumpSlice(px, py, pz);
      }
    }
  else if ( orientation == vtkSlicerVolumeResliceDriverLogic::ORIENTATION_TRANSVERSE )
    {
    if ( method == vtkSlicerVolumeResliceDriverLogic::METHOD_ORIENTATION )
      {
      sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 0);
      }
    else
      {
      sliceNode->SetOrientationToCoronal();
      sliceNode->JumpSlice(px, py, pz);
      }
    }
  sliceNode->UpdateMatrices();
}


void vtkSlicerVolumeResliceDriverLogic
::UpdateSliceIfObserved( vtkMRMLSliceNode* sliceNode )
{
  if ( sliceNode == NULL )
  {
    return;
  }
  
  const char* driverCC = sliceNode->GetAttribute( VOLUMERESLICEDRIVER_DRIVER_ATTRIBUTE );
  if ( driverCC == NULL )
  {
    return;
  }
  
  vtkMRMLNode* node = this->GetMRMLScene()->GetNodeByID( driverCC );
  
  sliceNode->Modified();
  node->InvokeEvent( vtkMRMLTransformableNode::TransformModifiedEvent );
}

