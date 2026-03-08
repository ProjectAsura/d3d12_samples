//-----------------------------------------------------------------------------
// File : RayQuery.hlsli
// Desc : Ray Query Wrapper.
// Copyright(c) Project Asura. All right reserved.
//-----------------------------------------------------------------------------
#ifndef RAY_QUERY_HLSLI
#define RAY_QUERY_HLSLI


#ifndef ENABLE_PROCEDURAL_PRIMITIVE
#define ENABLE_PROCEDURAL_PRIMITIVE (0)
#endif//ENABLE_PROCEDURAL_PRIMITIVE

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif//UNUSED

// TLASの定義.
typedef RaytracingAccelerationStructure Tlas;

///////////////////////////////////////////////////////////////////////////////
// HitRecord structure
///////////////////////////////////////////////////////////////////////////////
struct HitRecord
{
    float       Distance;           //!< ヒット距離.
    float2      Barycentrics;       //!< 重心座標.
    uint        InstanceIndex;      //!< インスタンス番号.
    uint        InstanceID;         //!< ユーザーインスタンスID.
    uint        HitGroupIndex;      //!< ヒットグループ番号.
    uint        GeometryIndex;      //!< ジオメトリ番号.
    uint        PrimitiveIndex;     //!< プリミティブ番号.
    bool        FrontFace;          //!< ヒットが前面かどうか?
    bool        ProceduralHit;      //!< プロシージャルプリミティブにヒットしたかどうか?
    float3x4    ObjectToWorld;      //!< オブジェクト空間からワールド空間への変換行列.
    
    bool IsMiss()
    { return Distance < 0; }

    bool IsHit()
    { return !IsMiss(); }

    float3 GetBarycentrics()
    { return float3(Barycentrics, saturate(1.0f - Barycentrics.x - Barycentrics.y)); }
};

///////////////////////////////////////////////////////////////////////////////
// PrimitiveArgs structure
///////////////////////////////////////////////////////////////////////////////
struct PrimitiveArgs
{
    uint InstanceIndex;     //!< インスタンス番号.
    uint InstanceID;        //!< ユーザーインスタンスID.
    uint HitGroupIndex;     //!< ヒットグループ番号.
    uint GeometryIndex;     //!< ジオメトリ番号.
    uint PrimitiveIndex;    //!< プリミティブ番号.
};

///////////////////////////////////////////////////////////////////////////////
// DefaultIntersectCallback structure
///////////////////////////////////////////////////////////////////////////////
struct DefaultIntersectCallback
{
    bool OnAnyHit
    (
        float3      rayOrg,     //!< レイの原点(オブジェクト空間).
        float3      rayDir,     //!< レイの方向ベクトル(オブジェクト空間).
        HitRecord   record      //!< ヒット情報.
    )
    {
        UNUSED(rayOrg);
        UNUSED(rayDir);
        UNUSED(record);
        return true;
    }
    
    bool OnPrimitive
    (
        float3          rayOrg, //!< レイの原点(オブジェクト空間).
        float3          rayDir, //!< レイの方向ベクトル(オブジェクト空間).
        float           tMin,   //!< 距離の最小値.
        inout float     tCur,   //!< 現在の距離.
        PrimitiveArgs   args    //!< プリミティブ情報.
    )
    {
        UNUSED(rayOrg);
        UNUSED(rayDir);
        UNUSED(tMin);
        UNUSED(tCurrent);
        UNUSED(args);
        return false;
    }
};

//-----------------------------------------------------------------------------
//      インラインレイトレーシングを実行します.
//-----------------------------------------------------------------------------
template<typename CallbackType>
HitRecord TraceRayWithCallback
(
    Tlas                tlas,
    uint                rayFlags,
    uint                instanceInclusionMask,
    RayDesc             ray,
    inout CallbackType  callback
)
{
    #if ENABLE_PROCEDURAL_PRIMITIVE
    RayQuery<RAY_FLAG_NONE> query;
    #else
    RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVE> query;
    #endif

    query.TraceRayInline(tlas, rayFlags, instanceInclusionMask, ray);
    
    while(query.Proceed())
    {
        switch(query.CandidateType())
        {
        #if ENABLE_PROCEDURAL_PRIMITIVE
        case CANDIDATE_PROCEDURAL_PRIMITIVE:
            {
                PrimitiveArgs args;
                args.InstanceID     = query.CandidateInstanceID();
                args.InstanceIndex  = query.CandidateInstanceIndex();
                args.PrimitiveIndex = query.CandidatePrimitiveIndex();
                args.GeometryIndex  = query.CandidateGeometryIndex();
                args.HitGroupIndex  = query.CandidateInstanceContributionToHitGroupIndex();
            
                const float3 rayOrg = query.CandidateObjectRayOrigin();
                const float3 rayDir = query.CandidateObjectRayDirection();
                const float  tMin   = query.RayTMin();
                const float  tMax   = query.ComittedRayT();
            
                float tCur = tMax;
                if (callback.OnPrimitive(rayOrg, rayDir, tMin, tCur, args))
                { query.CommitProceduralPrimitiveHit(tCur); }
            }
            break;
        #endif
        case CANDIDATE_NON_OPAQUE_TRIANGLE:
            {
                HitRecord candidate;
                candidate.Distance          = query.CandidateTriangleRayT();
                candidate.InstanceID        = query.CandidateInstanceID();
                candidate.InstanceIndex     = query.CandidateInstanceIndex();
                candidate.PrimitiveIndex    = query.CandidatePrimitiveIndex();
                candidate.GeometryIndex     = query.CandidateGeometryIndex();
                candidate.Barycentrics      = query.CandidateTriangleBarycentrics();
                candidate.FrontFace         = query.CandidateTriangleFrontFace();
                candidate.HitGroupIndex     = query.CandidateInstanceContributionToHitGroupIndex();
                candidate.ObjectToWorld     = query.CandidateObjectToWorld3x4();
                
                if (candidate.OnAnyHit(
                    query.CandidateObjectRayOrigin(),
                    query.CandidateObjectRayDirection(),
                    candiate))
                { query.CommitNonOpaqueTriangleHit(); }
            }
            break;
        }
    }

    HitRecord record = (HitRecord)0;
    record.Distance  = -1.0f;

    switch(query.CommittedStatus())
    {
    case COMMITTED_TRIANGLE_HIT:
        {
            record.Distance         = query.CommittedRayT();
            record.InstanceID       = query.CommittedInstanceID();
            record.InstanceIndex    = query.CommitedInstanceIndex();
            record.PrimitiveIndex   = query.CommittedPrimitiveIndex();
            record.GeometryIndex    = query.CommittedGeometryIndex();
            record.Barycentrics     = query.CommittedTriangleBarycentrics();
            record.FrontFace        = query.CommittedTriangleFrontFace();
            record.HitGroupIndex    = query.CommittedInstanceContributionToHitGroupIndex();
            record.ObjectToWorld    = query.CommittedObjectToWorld3x4();
        }
        break;
        
    #if ENABLE_PROCEDURAL_PRIMITIVE
    case COMMITTED_PROCEDURAL_PRIMITIVE_HIT:
        {
            record.ProceduralHit    = true;
            record.Distance         = query.CommittedRayT();
            record.InstanceID       = query.CommittedInstanceID();
            record.InstanceIndex    = query.CommittedInstanceIndex();
            record.PrimitiveIndex   = query.CommittedPrimitiveIndex();
            record.GeometryIndex    = query.CommittedGeometryIndex();
            record.Barycentrics     = 0.0f.xx;
            record.FrontFace        = true;
            record.HitGroupIndex    = query.CommittedInstanceContributionToHitGroupIndex();
            record.ObjectToWorld    = query.CommittedObjectToWorld3x4();
        }
        break;
    #endif
    }

    return record;
}

HitRecord TraceRay
(
    Tlas    tlas,
    uint    rayFlags,
    uint    instanceInclusionMask,
    RayDesc ray
)
{
    DefaultIntersectCallback callback;
    return TraceRayWithCallback(tlas, rayFlags, instanceInclusionMask, ray, callback);
}

#endif//RAY_QUERY_HLSLI
