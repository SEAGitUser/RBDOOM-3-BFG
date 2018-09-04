
#ifndef __RENDERENTITY_H__
#define __RENDERENTITY_H__

class idRenderLightLocal {
	friend class idRenderWorldLocal;
	friend void CreateMapLight( const idMapEntity* ); //SEA: temp temp
private:
	renderLightParms_t		parms;					// specification

													// first added, so the prelight model is not valid
	idRenderWorldLocal* 	world;
	int						index;					// in world lightdefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory.
	bool					archived;			// for demo writing
public:
	// derived information
	idRenderPlane			lightProject[ 4 ];		// old style light projection where Z and W are flipped and projected lights lightProject[3] is divided by ( zNear + zFar )
	idRenderMatrix			baseLightProject;		// global xyz1 to projected light strq
	idRenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space

	const idMaterial* 		lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage* 				falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel
	idBounds				globalLightBounds;

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	viewLight_t* 			viewLight;

	areaReference_t* 		references;				// each area the light is present in will have a lightRef
	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	struct doublePortal_t *	foggedPortals;

	int						areaNum;				// if not -1, we may be able to cull all the light's
													// interactions if !viewDef->connectedAreas[areaNum]

	bool					lightHasMoved;			// the light has changed its position since it was

public:
	idRenderLightLocal();

	ID_INLINE int						GetIndex() const { return index; }
	ID_INLINE idRenderWorldLocal *		GetOwner() const { return world; }

	ID_INLINE const renderLightParms_t & GetParms() const { return parms; }
	ID_INLINE const idVec3 &			GetOrigin() const { return parms.origin; }
	ID_INLINE const idMat3 &			GetAxis() const { return parms.axis; }
	ID_INLINE bool						LightCastsShadows() const { return parms.forceShadows || ( !parms.noShadows && lightShader->LightCastsShadows() ); }
	ID_INLINE int						GetID() const { return parms.lightId; }
	ID_INLINE bool						HasPreLightModel() const { return( parms.prelightModel != nullptr ); }

	float								GetAxialSize() const;
	void								DeriveData();
	void								FreeDerivedData();
	void								CreateReferences();

	// Creates viewLight_t with an empty scissor rect and links it to the RenderView.
	// Returns created viewLight_t pointer.
	viewLight_t *						EmitToView( idRenderView* );

	ID_INLINE bool CullModelBoundsToLight( const idBounds& localBounds, const idRenderMatrix& modelRenderMatrix ) const
	{
		idRenderMatrix modelLightProject;
		idRenderMatrix::Multiply( baseLightProject, modelRenderMatrix, modelLightProject );
		return idRenderMatrix::CullBoundsToMVP( modelLightProject, localBounds, true );
	}
};


class idRenderEntityLocal {
	friend class idRenderWorldLocal;
private:
	renderEntityParms_t		parms;

	idRenderWorldLocal* 	world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory.
	bool					archived;			// for demo writing

public:
	idRenderModel* 			dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
													// dynamicModel if this doesn't == tr.viewCount
	idRenderModel* 			cachedDynamicModel;

	// this is just a rearrangement of parms.axis and parms.origin
	idRenderMatrix			modelRenderMatrix;
	//idRenderMatrix			prevModelRenderMatrix;
	idRenderMatrix			inverseBaseModelProject;// transforms the unit cube to exactly cover the model in world space

													// the local bounds used to place entityRefs, either from parms for dynamic entities, or a model bounds
	idBounds				localReferenceBounds;

	// axis aligned bounding box in world space, derived from refernceBounds and
	// modelMatrix in R_CreateEntityRefs()
	idBounds				globalReferenceBounds;

	// a viewModel_t is created whenever a idRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if tr.viewCount == viewCount, viewModel is valid,
													// but the entity may still be off screen
	viewModel_t* 			viewModel;				// in frame temporary memory

	idRenderModelDecal* 	decals;					// decals that have been projected on this model
	idRenderModelOverlay* 	overlays;				// blood overlays on animated models

	areaReference_t* 		entityRefs;				// chain of all references

	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	bool					needsPortalSky;

public:
	idRenderEntityLocal();

	ID_INLINE int						GetIndex() const { return index; }
	ID_INLINE idRenderWorldLocal *		GetOwner() const { return world; }

	ID_INLINE const renderEntityParms_t & GetParms() const { return parms; }
	ID_INLINE const idVec3 &			GetOrigin() const { return parms.origin; }
	ID_INLINE const idMat3 &			GetAxis() const { return parms.axis; }
	ID_INLINE const idRenderModel *		GetModel() const { return parms.hModel; }

	void								ReadFromDemoFile( class idDemoFile* );
	void								WriteToDemoFile( class idDemoFile* ) const;

	bool								IsDirectlyVisible() const;

	void								DeriveData();
	void								FreeDerivedData( bool keepDecals, bool keepCachedDynamicModel );
	void								FreeDecals();
	void								FreeFadedDecals( int time );
	void								FreeOverlay();

	void								CreateReferences();

	void								ClearDynamicModel();
	bool								IssueCallback();
	idRenderModel *						EmitDynamicModel();
	ID_INLINE bool						HasCallback() const { return( parms.callback != nullptr ); }


	// Creates viewModel_t with an empty scissor rect and links it to the RenderView.
	// Returns created viewModel_t pointer.
	viewModel_t *						EmitToView( idRenderView* );
};

#endif /*__RENDERENTITY_H__*/