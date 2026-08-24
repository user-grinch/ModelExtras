#include "pch.h"
#include "frame.h"
#include <rw/rpworld.h>
#include <CVisibilityPlugins.h>

void FrameUtil::SetRotationX(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E00, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void FrameUtil::SetRotationY(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E0C, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

void FrameUtil::SetRotationZ(RwFrame *frame, float angle)
{
    RwFrameRotate(frame, (RwV3d *)0x008D2E18, (RwReal)angle, rwCOMBINEPRECONCAT);
    RwFrameUpdateObjects(frame);
}

uint32_t FrameUtil::GetChildCount(RwFrame *parent)
{
    RwFrame *child = parent->child;
    uint32_t count = 0U;
    if (child)
    {
        while (child)
        {
            ++count;
            child = child->next;
        }
        return count;
    }
    return 0U;
}

void FrameUtil::DestroyNodeHierarchyRecursive(RwFrame * frame)
{
    if (frame == nullptr)
    {
        return;
    }

    // Read the links before the frame goes away. RwFrameDestroy frees it and unlinks
    // it from its parent, so reaching for frame->child afterwards walks freed memory,
    // and whether the rest of the hierarchy gets destroyed is down to whatever lands
    // in that block next. The children have to go first anyway, RwFrameDestroy only
    // detaches the frame it's given.
    RwFrame * child = frame->child;
    RwFrame * next = frame->next;

    DestroyNodeHierarchyRecursive(child);
    DestroyNodeHierarchyRecursive(next);

    RpAtomic * atomic = (RpAtomic *)GetFirstObject(frame);
    if (atomic != nullptr)
    {
        RpClump * clump = atomic->clump;
        RpClumpRemoveAtomic(clump, atomic);
        RpAtomicDestroy(atomic);
    }

    RwFrameDestroy(frame);
}

void FrameUtil::HideAllAtomicsExcept(RwFrame *frame, int indexToKeep)
{
    if (frame && !rwLinkListEmpty(&frame->objectList))
    {
        RwObjectHasFrame *atomic;
        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);
        int currentIndex = 0;

        while (current != end)
        {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);

            if (currentIndex == indexToKeep)
                atomic->object.flags |= rpATOMICRENDER;
            else
                atomic->object.flags &= ~rpATOMICRENDER;

            current = rwLLLinkGetNext(current);
            currentIndex++;
        }
    }
}

void FrameUtil::StoreChilds(RwFrame *parent, std::vector<RwFrame *> &store)
{
    RwFrame *child = parent->child;

    while (child)
    {
        store.push_back(child);
        child = child->next;
    }
}

void FrameUtil::ShowAllAtomics(RwFrame *frame)
{
    if (!rwLinkListEmpty(&frame->objectList))
    {
        RwObjectHasFrame *atomic;

        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);

        do
        {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags |= rpATOMICRENDER; // clear

            current = rwLLLinkGetNext(current);
        } while (current != end);
    }
}

void FrameUtil::HideAllAtomics(RwFrame *frame)
{
    if (!rwLinkListEmpty(&frame->objectList))
    {
        RwObjectHasFrame *atomic;

        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);

        while (current != end)
        {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            atomic->object.flags &= ~rpATOMICRENDER;

            current = rwLLLinkGetNext(current);
        }
    }
}

void FrameUtil::HideChildWithName(RwFrame *parent_frame, const char *name)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        if (!strcmp(GetFrameNodeName(child), name))
        {
            FrameUtil::HideAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void FrameUtil::ShowChildWithName(RwFrame *parent_frame, const char *name)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        if (!strcmp(GetFrameNodeName(child), name))
        {
            FrameUtil::ShowAllAtomics(child);
            return;
        }
        child = child->next;
    }
}

void FrameUtil::HideAllChilds(RwFrame *parent_frame)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        FrameUtil::HideAllAtomics(child);
        child = child->next;
    }
    FrameUtil::HideAllAtomics(parent_frame);
}

void FrameUtil::ShowAllChilds(RwFrame *parent_frame)
{
    RwFrame *child = parent_frame->child;
    while (child)
    {
        FrameUtil::ShowAllAtomics(child);
        child = child->next;
    }
    FrameUtil::ShowAllAtomics(parent_frame);
}

bool FrameUtil::IsAtomicVisible(RwFrame* frame) {
    if (!frame) return true;

    if (!rwLinkListEmpty(&frame->objectList)) {
        RwObjectHasFrame *atomic;
        RwLLLink *current = rwLinkListGetFirstLLLink(&frame->objectList);
        RwLLLink *end = rwLinkListGetTerminator(&frame->objectList);

        bool hasAtomics = false;
        do {
            atomic = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);
            hasAtomics = true;
            if (atomic->object.flags & rpATOMICRENDER) {
                return true;
            }

            current = rwLLLinkGetNext(current);
        } while (current != end);

        if (hasAtomics) {
            return false;
        }
    }

    return true;
}

bool FrameUtil::IsOkAtomicVisible(RwFrame* frame) {
    if (!frame) return true;

    if (!IsAtomicVisible(frame)) {
        return false;
    }

    // Check if this component dummy has child damage parts (e.g. bump_front_ok, wing_lf_ok)
    RwFrame *child = frame->child;
    while (child) {
        const char *childName = GetFrameNodeName(child);
        if (childName && strstr(childName, "_ok")) {
            if (!IsAtomicVisible(child)) {
                return false;
            }
        }
        child = child->next;
    }

    return true;
}

// VehFuncs
RwFrame * FrameUtil::Clone(RwFrame *frame, RpClump *clump, RwFrame *parent, bool isRoot) {
	RwFrame * newFrame = parent;
	if (isRoot) {
		*(uint32_t*)0xC1CB58 = (uint32_t)clump;
		RwFrameForAllObjects(frame, CopyObjectsCB, parent);
		if (RwFrame * nextFrame = frame->child) Clone(nextFrame, clump, parent, false);
	} else {
		newFrame = RwFrameCreate();

		//CVisibilityPlugins::SetFrameHierarchyId(newFrame, 101);

		memcpy(&newFrame->modelling, &frame->modelling, sizeof(RwMatrix));
		RwMatrixUpdate(&newFrame->modelling);

		const std::string frameName = GetFrameNodeName(frame);
		SetFrameNodeName(newFrame, &frameName[0]);

		RpAtomic * atomic = (RpAtomic*)GetFirstObject(frame);

		if (atomic) {
			RpAtomic * newAtomic = RpAtomicClone(atomic);
			RpAtomicSetFrame(newAtomic, newFrame);
			RpClumpAddAtomic(clump, newAtomic);
			//CVisibilityPlugins::SetAtomicRenderCallback(newAtomic, (RpAtomic *(*)(RpAtomic *))0x007323C0);
		}

		RwFrameAddChild(parent, newFrame);

		if (RwFrame * nextFrame = frame->child) Clone(nextFrame, clump, newFrame, false);
		// if (RwFrame * nextFrame = frame->next)  CloneNode(nextFrame, clump, parent, false);
	}
	return newFrame;
}