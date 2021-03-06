#include "StdAfx.h"
#include "SphsereMeshPlaneMeshQuery.h"
#include "Terrain.h"
#include "TXSurfaceDrawer.h"
#include "TXOpcodeModelBuilder.h"
#include "../graphstructure/TXPatchDetector.h"
#include "../tessellationtri/subdivisionlevel.h"
#include "../tessellationtri/txFrustumSubdivision.h"
#include "../txShapelib/txTriSurfaceData.h"
#include "../txShapelib/txFrustum.h"


SphsereMeshPlaneMeshQuery::SphsereMeshPlaneMeshQuery(void)
{
}


SphsereMeshPlaneMeshQuery::~SphsereMeshPlaneMeshQuery(void)
{
}

void SphsereMeshPlaneMeshQuery::Init()
{
	surface = new SurfaceImporter();
	assert(surface);
	// Initialize from OFF file
	// char *filename = "E:\\motionsolve\\ProjectTime\\20130326\\meshlib\\meshes\\bunny_nf500.noff";
	// char *filename = "E:\\motionsolve\\ProjectTime\\20130326\\meshlib\\meshes\\tetra.off";
	// char *filename = "E:\\motionsolve\\ProjectTime\\20130326\\meshlib\\meshes\\sphere3.off";
	// char *filename = "E:\\motionsolve\\ProjectTime\\20130326\\meshlib\\meshes\\cube.off";
	// char *filename = "E:\\motionsolve\\ProjectTime\\20130408\\trimesh.off";
	// 
	//surface->InitializeFromOFF(filename);
	//mesh = surface->GetMesh();

	// Initialize from subdivision surface.
	// construct subvision
	txTessellateSphere(1.0,3,1,surf);
	double r[3] = {0.0,0.0,-0.7};
	surface->InitializeFromOFFSurface(surf,r,NULL);
	mesh = surface->GetMesh();

	drawer = new TXSurfaceDrawer(surface);
	assert(drawer);

	modelbuilder = new TXOpcodeModelBuilder(surface);
	assert(modelbuilder);

	// for surface 2
	surface2 = new SurfaceImporter();
	assert(surface2);

	txFrustum frustum(5.0, 5.0, 1.0);
	txFrustumSubdivision subfrustum(&frustum,30);
	subfrustum.Subdivision();
	surf2 = new txTriSurfaceData(*subfrustum.GetTriSurfData());

	surface2->InitializeFromOFFSurface(surf2);
	mesh2 = surface2->GetMesh();

	drawer2 = new TXSurfaceDrawer(surface2);
	assert(drawer2);

	modelbuilder2 = new TXOpcodeModelBuilder(surface2);
	assert(modelbuilder2);

	// With Patch-Finder
	pfappset = WITHPATCHFINDER;

}

void SphsereMeshPlaneMeshQuery::Release()
{
	delete surface;
	surface = NULL;
	delete drawer;
	drawer = NULL;
	delete modelbuilder;
	modelbuilder = NULL;
	delete surface2;
	surface2 = NULL;
	delete drawer2;
	drawer2 = NULL;
	delete modelbuilder2;
	modelbuilder2 = NULL;
	delete surf2;
}

void SphsereMeshPlaneMeshQuery::PerformTest()
{

	/**
	* The folowing code is a test:
	* You cannot do this in stl even you allocate the memeory first.
	*/
	//std::vector<int> intlist;
	//intlist.reserve(3);
	//intlist[2] = 5;
	//intlist[1] = 4;
	//intlist[0] = 3;

	// Draw the Tri-Mesh
	drawer->Draw(true,false);
	drawer2->Draw(true,false);
	//RenderTerrain();

	const Model *TM2 = GetTerrainModel();
	const Model *spheremodel = modelbuilder->GetModel();

	// set collider which will be move to the class
	Opcode::AABBTreeCollider collider;
	collider.SetFirstContact(false);
	collider.SetFullPrimBoxTest(false);

	Opcode::BVTCache cache;
	cache.Model0 = spheremodel;
	//cache.Model1 = TM2;
	cache.Model1 = modelbuilder2->GetModel();

	Matrix4x4 matplane;
	// vertical
	//matplane.Set(
	//	1.0,0.0,0.0,0.0,
	//	0.0,0.0,-1.0,0.0,
	//	0.0,1.0,0.0,0.0,
	//	0.0,0.0,0.0,1.0
	//	);

	matplane.Identity();


	assert(collider.ValidateSettings()==0);

	//Pair cdpair;

	if (collider.Collide(cache,null,&matplane))
	{
		if (collider.GetContactStatus())
		{
			//udword NbTris = collider.GetNbTouchedPrimitives();
			//const udword* Indices = collider.GetTouchedPrimitives();
			size_t *indexes = new size_t[collider.GetNbPairs()];
			size_t *indexes2 = new size_t[collider.GetNbPairs()];
			int numcollider = collider.GetNbPairs();
			const Pair *opcpairs = collider.GetPairs();
			for (size_t i=0; i<collider.GetNbPairs(); i++)
			{
				indexes[i]  = opcpairs[i].id0;
				indexes2[i] = opcpairs[i].id1;
				//indexes[i+1] = opcpairs[i].id1+1;
				//indexes[i+2] = opcpairs[i].id1+2;
			}

			// RenderTerrainTriangles(collider.GetNbPairs(), indexes2);
			Point color(1.0,0.0,0.0);
			//RenderSurfaceTriangles(surface2,collider.GetNbPairs(),indexes2, &color);
			RenderSurfaceTriangles(surface,collider.GetNbPairs(),indexes, &color);

			if (pfappset==WITHPATCHFINDER) 
			{
				// detect the missing triangle
				TXPatchDetector patchdetector(mesh);
				patchdetector.InputRing(collider.GetNbPairs(), indexes);
				patchdetector.DetectPath();
				size_t *patchindices = patchdetector.PatchIndeces1();
				// Rnder the patch face patch
				Point color(1.0,0.0,0.0);
				RenderSurfaceTriangles(surface, patchdetector.PatchCount1(), patchindices, &color);

			} else {
			
			}

			delete [] indexes;
			indexes = NULL;
			delete [] indexes2;
			indexes2 = NULL;
		}
	}
}

void SphsereMeshPlaneMeshQuery::Select()
{
	{
		mBar = TwNewBar("Patch-Finder Tests");
		TwEnumVal testPatchFinder[NUM_PF_STATES] = {
			{WITHPATCHFINDER, "With patch-finder"},
			{WITHOUTPATCHFINDER, "Without patch-finder"},
		};
		TwType pfType = TwDefineEnum("Patch-Finder Tests", testPatchFinder, NUM_PF_STATES);
		TwAddVarRW(mBar,"Patch-Finder Tests",pfType,&pfappset,"");
	}
}

void SphsereMeshPlaneMeshQuery::Deselect()
{
	if (mBar)
	{
		TwDeleteBar(mBar);
		mBar = null;
	}
}

void SphsereMeshPlaneMeshQuery::KeyboardCallback(unsigned char key, int x, int y)
{
}

void SphsereMeshPlaneMeshQuery::MouseCallback(int button, int state, int x, int y)
{
}

void SphsereMeshPlaneMeshQuery::MotionCallback(int x, int y)
{
}

