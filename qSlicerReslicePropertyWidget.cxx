
// Qt includes
#include <QButtonGroup>

#include "qSlicerReslicePropertyWidget.h"
#include "ui_qSlicerReslicePropertyWidget.h"

// MRMLWidgets includes
#include <qMRMLSliceWidget.h>
#include <qMRMLSliceControllerWidget.h>
#include <qMRMLThreeDWidget.h>
#include <qMRMLThreeDViewControllerWidget.h>

// MRML includes
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLViewNode.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScalarVolumeNode.h"

// MRMLLogic includes
#include "vtkMRMLLayoutLogic.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkSmartPointer.h>
#include <vtkMatrix4x4.h>
#include <vtkImageData.h>


class qSlicerReslicePropertyWidgetPrivate;
class vtkMRMLNode;
class vtkObject;


//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qSlicerReslicePropertyWidgetPrivate : public Ui_qSlicerReslicePropertyWidget
{
  Q_DECLARE_PUBLIC(qSlicerReslicePropertyWidget);
protected:
  qSlicerReslicePropertyWidget* const q_ptr;
public:
  qSlicerReslicePropertyWidgetPrivate(qSlicerReslicePropertyWidget& object);
  void init();
  void updateSlice(vtkMatrix4x4* transform);
  void updateSliceByTransformNode(vtkMRMLLinearTransformNode* tnode);
  void updateSliceByImageNode(vtkMRMLScalarVolumeNode* inode);

  QButtonGroup methodButtonGrouop;
  QButtonGroup orientationButtonGroup;

  enum {
    METHOD_POSITION,
    METHOD_ORIENTATION,
  };
  
  enum {
    ORIENTATION_INPLANE,
    ORIENTATION_INPLANE90,
    ORIENTATION_TRANSVERSE,
  };

  vtkMRMLScene * scene;
  vtkMRMLSliceNode * sliceNode;
  vtkMRMLNode  * driverNode;

};

//------------------------------------------------------------------------------
qSlicerReslicePropertyWidgetPrivate::qSlicerReslicePropertyWidgetPrivate(qSlicerReslicePropertyWidget& object)
  : q_ptr(&object)
{
  scene = NULL;
  sliceNode = NULL;
  driverNode = NULL;
}

//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidgetPrivate::init()
{
  Q_Q(qSlicerReslicePropertyWidget);
  this->setupUi(q);

  //QObject::connect(this->ConnectorNameEdit, SIGNAL(editingFinished()),
  //                 q, SLOT(updateIGTLConnectorNode()));
  //QObject::connect(this->ConnectorStateCheckBox, SIGNAL(toggled(bool)),
  //                 q, SLOT(startCurrentIGTLConnector(bool)));
  //QObject::connect(this->ConnectorHostNameEdit, SIGNAL(editingFinished()),
  //                 q, SLOT(updateIGTLConnectorNode()));
  //QObject::connect(this->ConnectorPortEdit, SIGNAL(editingFinished()),
  //                 q, SLOT(updateIGTLConnectorNode()));
  //QObject::connect(&this->ConnectorTypeButtonGroup, SIGNAL(buttonClicked(int)),
  //                 q, SLOT(updateIGTLConnectorNode()));
  //

  this->methodButtonGrouop.addButton(this->positionRadioButton, METHOD_POSITION);
  this->methodButtonGrouop.addButton(this->orientationRadioButton, METHOD_ORIENTATION);
  this->positionRadioButton->setChecked(true);
  this->orientationButtonGroup.addButton(this->inPlaneRadioButton, ORIENTATION_INPLANE);
  this->orientationButtonGroup.addButton(this->inPlane90RadioButton, ORIENTATION_INPLANE90);
  this->orientationButtonGroup.addButton(this->transverseRadioButton, ORIENTATION_TRANSVERSE);
  this->inPlaneRadioButton->setChecked(true);
  
  QObject::connect(this->driverNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                   q, SLOT(setDriverNode(vtkMRMLNode*)));
  

}

//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidgetPrivate::updateSlice(vtkMatrix4x4 * transform)
{
  //Q_Q(qSlicerReslicePropertyWidget);

  float tx = transform->Element[0][0];
  float ty = transform->Element[1][0];
  float tz = transform->Element[2][0];
  float nx = transform->Element[0][2];
  float ny = transform->Element[1][2];
  float nz = transform->Element[2][2];
  float px = transform->Element[0][3];
  float py = transform->Element[1][3];
  float pz = transform->Element[2][3];

  if (orientationButtonGroup.checkedId() == ORIENTATION_INPLANE)
    {
    if (this->methodButtonGrouop.checkedId() == METHOD_ORIENTATION)
      {
      this->sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 0);
      }
    else
      {
      this->sliceNode->SetOrientationToAxial();
      this->sliceNode->JumpSlice(px, py, pz);
      }
    }
  else if (orientationButtonGroup.checkedId() == ORIENTATION_INPLANE90)
    {
    if (this->methodButtonGrouop.checkedId() == METHOD_ORIENTATION)
      {
      this->sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 1);
      }
    else
      {
      this->sliceNode->SetOrientationToSagittal();
      this->sliceNode->JumpSlice(px, py, pz);
      }
    }
  else if (orientationButtonGroup.checkedId() == ORIENTATION_TRANSVERSE)
    {
    if (this->methodButtonGrouop.checkedId() == METHOD_ORIENTATION) 
      {
      this->sliceNode->SetSliceToRASByNTP(nx, ny, nz, tx, ty, tz, px, py, pz, 2);
      }
    else
      {
      this->sliceNode->SetOrientationToCoronal();
      this->sliceNode->JumpSlice(px, py, pz);
      }
    }
  this->sliceNode->UpdateMatrices();
}


//---------------------------------------------------------------------------
void qSlicerReslicePropertyWidgetPrivate::updateSliceByTransformNode(vtkMRMLLinearTransformNode* tnode)
{
  Q_Q(qSlicerReslicePropertyWidget);

  if (!tnode)
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> transform = vtkMatrix4x4::New();
  if (transform)
    {
    transform->Identity();
    int getTransf = tnode->GetMatrixTransformToWorld(transform);
    if(getTransf != 0)
      {
      this->updateSlice(transform);
      }
    }
}


//---------------------------------------------------------------------------
void qSlicerReslicePropertyWidgetPrivate::updateSliceByImageNode(vtkMRMLScalarVolumeNode* inode)
{
  Q_Q(qSlicerReslicePropertyWidget);

  vtkMRMLVolumeNode* volumeNode = inode;

  if (volumeNode == NULL)
    {
    return;
    }

  vtkSmartPointer<vtkMatrix4x4> rtimgTransform = vtkMatrix4x4::New();
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

  updateSlice(rtimgTransform);

}



//------------------------------------------------------------------------------
qSlicerReslicePropertyWidget::qSlicerReslicePropertyWidget(QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerReslicePropertyWidgetPrivate(*this))
{
  Q_D(qSlicerReslicePropertyWidget);
  d->init();
}

//------------------------------------------------------------------------------
qSlicerReslicePropertyWidget::~qSlicerReslicePropertyWidget()
{
}

//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidget::setSliceViewName(const QString& newSliceViewName)
{
  this->setTitle(newSliceViewName);
}

//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidget::setMRMLSliceNode(vtkMRMLSliceNode* newSliceNode)
{
  Q_D(qSlicerReslicePropertyWidget);

  //d->SliceLogic->SetSliceNode(newSliceNode);
  d->sliceNode = newSliceNode;
  if (newSliceNode && newSliceNode->GetScene())
    {
    this->setMRMLScene(newSliceNode->GetScene());
    }
}

//----------------------------------------------------------------------------
void qSlicerReslicePropertyWidget::setMRMLScene(vtkMRMLScene * newScene)
{
  Q_D(qSlicerReslicePropertyWidget);

  if (d->scene != newScene)
    {
    d->scene = newScene;
    if (d->driverNodeSelector)
      {
      d->driverNodeSelector->setMRMLScene(newScene);
      }
    }
}


//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidget::setDriverNode(vtkMRMLNode * newNode)
{
  Q_D(qSlicerReslicePropertyWidget);

  if (d->driverNode != newNode)
    {
    vtkMRMLLinearTransformNode * tnode = vtkMRMLLinearTransformNode::SafeDownCast(newNode);
    if (tnode)
      {
      // reconnect current event listener
      qvtkReconnect(d->driverNode, newNode,
                    vtkMRMLTransformableNode::TransformModifiedEvent,
                    this, SLOT(onMRMLNodeModified()));
      }

    vtkMRMLScalarVolumeNode * inode = vtkMRMLScalarVolumeNode::SafeDownCast(newNode);
    if (inode)
      {
      qvtkReconnect(d->driverNode, newNode,
                    vtkMRMLVolumeNode::ImageDataModifiedEvent,
                    this, SLOT(onMRMLNodeModified()));
      }
    d->driverNode = newNode;
    }
}


//------------------------------------------------------------------------------
void qSlicerReslicePropertyWidget::onMRMLNodeModified()
{
  Q_D(qSlicerReslicePropertyWidget);
  if (!d->driverNode)
    {
    return;
    }
  if (d->sliceNode)
    {
    vtkMRMLLinearTransformNode* transNode =
      vtkMRMLLinearTransformNode::SafeDownCast(d->driverNode);
    if (transNode)
      {
      d->updateSliceByTransformNode(transNode);
      }
    vtkMRMLScalarVolumeNode* imageNode = 
      vtkMRMLScalarVolumeNode::SafeDownCast(d->driverNode);
    if (imageNode)
      {
      d->updateSliceByImageNode(imageNode);
      }
    }
}
