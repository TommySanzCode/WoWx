"""Customization acceptance using rendered telemetry, not UI selection alone."""

def customization_checks(ready, plan):
    identities={race|(sex<<8) for race in range(1,9) for sex in range(2)}
    samples=plan.get('samples',[])
    keys={(s['identity'],s['row']) for s in samples}
    expected={(identity,row) for identity in identities for row in range(5)}
    checks={'customization_plan_complete':plan.get('version')==1 and len(samples)==80 and keys==expected}
    if not checks['customization_plan_complete']:
        return checks,[]
    appearance=[r for r in ready if r['character_screen']=='6' and int(r.get('preview_compositions',0))>0 and int(r.get('preview_atlas_hash',0))>0]
    missing=[];revisions={identity:[] for identity in identities}
    for sample in samples:
        matched=[r for r in appearance if int(r['preview_identity'])==sample['identity']
                 and int(r.get('character_appearance_row',-1))==sample['row']
                 and int(r.get('preview_look',-1))==sample['look'] and int(r.get('preview_facial',-1))==sample['facial']
                 and sample['frame']<=int(r['replay_frame'])<sample['frame']+60]
        if len(matched)<3:
            missing.append(sample)
        else:
            revisions[sample['identity']].append(int(matched[-1]['preview_compositions']))
    checks['all_80_customizations_drawn']=not missing
    checks['each_choice_recomposed']=all(len(values)==5 and all(a<b for a,b in zip(values,values[1:])) for values in revisions.values())
    randomized=set()
    for r in appearance:
        identity=int(r['preview_identity']);values=revisions.get(identity,[])
        expected_randomizations=((identity&255)-1)*2+(identity>>8)+1
        if (values and int(r.get('character_randomizations',0))==expected_randomizations
                and int(r['preview_compositions'])>max(values)):
            randomized.add(identity)
    checks['all_16_randomized_looks_drawn']=randomized==identities
    # The same appearance/outfit must not change texture as its pose animates.
    hashes={}
    for r in appearance:
        key=tuple(r.get(k) for k in ('preview_identity','preview_class','preview_look','preview_facial'))
        hashes.setdefault(key,set()).add(r['preview_atlas_hash'])
    checks['composed_pixels_stable_between_changes']=bool(hashes) and all(len(values)==1 for values in hashes.values())
    return checks,missing
