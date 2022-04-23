
#include "CorePrivate.h"

UComponent* UObject::FindComponent(FName ComponentName, UBOOL bRecurse)
{
	UComponent* Result = NULL;

	if (GetClass()->HasAnyClassFlags(CLASS_HasComponents))
	{
		TArray<UComponent*> ComponentReferences;

		UObject* ComponentRoot = this;

		/**
		 * Currently, a component is instanced only if the Owner [passing into InstanceComponents] is the Outer for the
		 * component that is being instanced.  The following loop allows components to be instanced the first time a reference
		 * is encountered, regardless of whether the current object is the component's Outer or not.
		 */
		while (ComponentRoot->GetOuter() && ComponentRoot->GetOuter()->GetClass() != UPackage::StaticClass())
		{
			ComponentRoot = ComponentRoot->GetOuter();
		}

		TArchiveObjectReferenceCollector<UComponent> ComponentCollector(&ComponentReferences,
			ComponentRoot,				// Required Outer
			FALSE,						// bRequireDirectOuter
			TRUE,						// bShouldIgnoreArchetypes
			bRecurse					// bDeepSearch
		);

		Serialize(ComponentCollector);
		for (INT CompIndex = 0; CompIndex < ComponentReferences.Num(); CompIndex++)
		{
			UComponent* Component = ComponentReferences(CompIndex);
			if (Component->TemplateName == ComponentName)
			{
				Result = Component;
				break;
			}
		}

		if (Result == NULL && HasAnyFlags(RF_ClassDefaultObject))
		{
			// see if this component exists in the class's component map
			UComponent** TemplateComponent = GetClass()->ComponentNameToDefaultObjectMap.Find(ComponentName);
			if (TemplateComponent != NULL)
			{
				Result = *TemplateComponent;
			}
		}
	}

	return Result;
}

/**
 * Find the subobject default object in the defaultactorclass, or in some base class.
 * This function goes away once the version is incremented.
 *
 * @return The object pointed to by the SourceDefaultActorClass and SourceDefaultSubObjectName
 */
UComponent* UComponent::ResolveSourceDefaultObject()
{
	FName ComponentName = TemplateName != NAME_None
		? TemplateName
		: GetFName();

	UComponent* Result = GetOuter()->GetArchetype()->FindComponent(ComponentName, TRUE);

	// this can probably go away
	/*UComponent* ComponentPtr = NULL;
	if (TemplateOwnerClass != NULL)
	{
		// look in the class's map for a subobject with the saved name
		ComponentPtr = TemplateOwnerClass->ComponentNameToDefaultObjectMap.FindRef(ComponentName);
	}*/

	return Result;
}

void UComponent::PreSerialize(FArchive& Ar)
{
	if (!GetOuter()->IsTemplate(RF_ClassDefaultObject))
		return;

	// load or save the pointer to the template (via class and name of sub object)
	//@fixme components - this should no longer be necessary
	Ar << TemplateOwnerClass;

	// serialize the TemplateName if this component is a subobject of a class default object
	// or this isn't a persistent array or we're duplicating an object
	Ar << TemplateName;

	// if we are loading, we need to copy the template onto us
	// SourceDefaultActorClass is NULL if this is the templated object, or if the components
	// was created in the level package (ie created via en EditInlineNew operation in the prop
	// broswer). In either case, there is no need to copy defaults.

	//@fixme components - figure out if this is still needed for anything
	// RF_ZombieComponent is definitely not needed anymore, since components are no longer late-bound to their templates
	// all components now have a direct reference to their template
	if (Ar.IsLoading() && Ar.IsPersistent())
	{
		if (TemplateOwnerClass == NULL)
		{
			// this check is intended to fix components of actors contained within prefabs which have somehow lost their archetype pointers...
			if (TemplateName == NAME_None && IsTemplate())
			{
				UObject* SourceDefaultObject = ResolveSourceDefaultObject();
				if (SourceDefaultObject != NULL)
				{
					// make sure the source object is fully serialized
					Ar.Preload(SourceDefaultObject);

					debugf(TEXT("%s: Restoring archetype reference to '%s'"), *GetFullName(), *SourceDefaultObject->GetPathName());
					SetArchetype(SourceDefaultObject, TRUE);
				}
			}
		}
		else
		{
			if (GetArchetype() == GetClass()->GetDefaultObject())
			{
				// first make sure that the template is fully serialized
				Ar.Preload(TemplateOwnerClass); // does this guarantee it's subojects are loaded?

				// hunt down the subobject pointed to by the class/name loaded above
				UObject* SourceDefaultObject = ResolveSourceDefaultObject();

				if (SourceDefaultObject == NULL)
				{
					// mark those object to be destroyed after loading is complete (to not break serialization)
					SetFlags(RF_ZombieComponent);
					return;
				}

				if (SourceDefaultObject != GetArchetype())
				{
					// make sure the source object is fully serialized
					Ar.Preload(SourceDefaultObject);

					// if our source object changed classes on us, we can't do anything
					if (GetClass() != SourceDefaultObject->GetClass())
					{
						debugf(TEXT("The source default object (%s) isn't the same class as the instance (%s)"), *SourceDefaultObject->GetFullName(), *GetFullName());
						return;
					}

					SetArchetype(SourceDefaultObject, TRUE);
				}
			}
		}
	}
}

void UComponent::LinkToSourceDefaultObject(UComponent* OriginalComponent, UClass* OwnerClass, FName ComponentName)
{
	if (GetArchetype()->GetOuter() != GetOuter()->GetArchetype())
	{
		UComponent* RealArchetype = ResolveSourceDefaultObject();

		if (RealArchetype != NULL)
			SetArchetype(RealArchetype, TRUE);
		else SetArchetype(GetClass()->GetDefaultObject(), TRUE);
	}
}

IMPLEMENT_CLASS(UComponent);
IMPLEMENT_CLASS(UDistributionFloat);
IMPLEMENT_CLASS(UDistributionVector);
