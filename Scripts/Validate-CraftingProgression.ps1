[CmdletBinding()]
param(
    [string]$CatalogPath = (Join-Path $PSScriptRoot '..\docs\crafting-progression\catalog.json'),
    [switch]$SelfTest,
    [switch]$SkipSourceParity
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent

function Assert-That($Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Assert-PositiveInteger($Value, [string]$Context) {
    Assert-That (($Value -is [int] -or $Value -is [long]) -and $Value -gt 0) "quantity: $Context must be a positive integer."
}

function Assert-StockEqual($Actual, $Expected, [string]$Context) {
    foreach ($key in @($Actual.Keys) + @($Expected.Keys) | Sort-Object -Unique) {
        $left = if ($Actual.Contains($key)) { $Actual[$key] } else { 0 }
        $right = if ($Expected.Contains($key)) { $Expected[$key] } else { 0 }
        Assert-That ($left -eq $right) "$Context : $key expected $right, got $left."
    }
}

function Add-Stock($Stock, $Delta, [int]$Multiplier = 1) {
    foreach ($key in $Delta.Keys) {
        if (-not $Stock.Contains($key)) { $Stock[$key] = 0 }
        $Stock[$key] += $Delta[$key] * $Multiplier
        Assert-That ($Stock[$key] -ge 0) "stock: insufficient $key."
    }
}

function Get-Capacity($Stock, $Entities) {
    $used = 0
    foreach ($key in $Stock.Keys) { $used += $Stock[$key] * $Entities[$key].capacity }
    return $used
}

function Test-References($Stock, $Entities, [string]$Context) {
    foreach ($key in $Stock.Keys) {
        Assert-That ($Entities.Contains($key)) "reference: unknown $key in $Context."
        Assert-PositiveInteger $Stock[$key] "$Context/$key"
    }
}

function Get-Index($Records, [string]$Context) {
    $index = [ordered]@{}
    foreach ($record in $Records) {
        Assert-That (-not $index.Contains($record.key)) "duplicate: $Context/$($record.key)."
        $index[$record.key] = $record
    }
    return $index
}

function Assert-Capacity($Pack, $Stored, $Entities, [int]$Limit) {
    $packUsed = Get-Capacity $Pack $Entities
    $storedUsed = Get-Capacity $Stored $Entities
    Assert-That ($packUsed -le $Limit) "capacity: pack $packUsed exceeds $Limit; no output may be discarded."
    Assert-That ($storedUsed -le $Limit) "capacity: chest $storedUsed exceeds $Limit."
    return $packUsed
}

function Test-Catalog($Data) {
    Assert-That ($Data.schemaVersion -eq 1) 'schema: expected version1.'
    Assert-That ($Data.units.inventoryCapacity -eq 120) 'capacity: current inventory must remain120.'
    $entities = Get-Index $Data.entities 'entity'
    $actions = Get-Index $Data.actions 'action'
    $ranks = @{current=0; next=1; later=2}
    foreach ($entity in $Data.entities) {
        Assert-That ($ranks.ContainsKey($entity.status)) "status: $($entity.key)."
        Assert-That ($entity.capacity -in @(0,1)) "capacity: $($entity.key) must explicitly use0 or1."
        if ($entity.runtimeEnum) {
            Assert-That ($entity.status -eq 'current') "id: proposed $($entity.key) assigned a runtime enum."
            $names = $Data.enums[$entity.runtimeEnum]
            Assert-That ($entity.runtimeId -ge 0 -and $entity.runtimeId -lt $names.Count) "id: invalid $($entity.key)."
            Assert-That ($entity.key.Split(':')[1] -ceq $names[$entity.runtimeId]) "id: identifier mismatch $($entity.key)."
        } else {
            Assert-That ($null -eq $entity.runtimeId) "id: proposed/derived $($entity.key) assigned an ID."
        }
    }
    foreach ($action in $Data.actions) {
        Assert-That ($ranks.ContainsKey($action.status)) "status: $($action.key)."
        Test-References $action.inputs $entities "$($action.key) inputs"
        Test-References $action.outputs $entities "$($action.key) outputs"
        foreach ($key in @($action.requires) + @($action.inputs.Keys) + @($action.outputs.Keys)) {
            Assert-That ($entities.Contains($key)) "reference: $($action.key) requires unknown $key."
            Assert-That ($ranks[$entities[$key].status] -le $ranks[$action.status]) "status: $($action.key) depends on future $key."
        }
        foreach ($field in @('authorityGameHours','nativeAdvanceGameHours','directEnergyPoints','proposedGameHours','passiveGameHours')) {
            if ($action.Contains($field)) {
                $value = $action[$field]
                Assert-That (($value -is [ValueType]) -and [double]::IsFinite([double]$value) -and $value -ge 0) "time: $($action.key)/$field."
            }
        }
        if ($action.status -ne 'current') {
            Assert-That ($action.Contains('missing') -and $action.missing.Length -gt 0) "status: $($action.key) needs missing-runtime notes."
            Assert-That ($action.Contains('proposedGameHours')) "time: $($action.key) needs proposed game hours."
        }
        if ($action.Contains('runtimeEnum')) {
            Assert-That ($action.status -eq 'current') "id: future action $($action.key)."
            $names = $Data.enums[$action.runtimeEnum]
            Assert-That ($action.runtimeId -ge 0 -and $action.runtimeId -lt $names.Count) "id: $($action.key)."
            Assert-That ($action.key.Split(':')[1] -ceq $names[$action.runtimeId]) "id: action mismatch $($action.key)."
        }
    }
    Test-References $Data.starting.inventory $entities 'starting inventory'
    foreach ($key in @($Data.starting.equipped) + @($Data.starting.conditions)) {
        Assert-That ($entities.Contains($key)) "reference: starting $key."
    }
    foreach ($stage in @('current','next','later')) {
        $available = @{}
        foreach ($key in @($Data.starting.inventory.Keys) + @($Data.starting.equipped) + @($Data.starting.conditions)) {
            $available[$key] = $true
        }
        do {
            $changed = $false
            foreach ($action in $Data.actions | Where-Object { $ranks[$_.status] -le $ranks[$stage] }) {
                $blocked = @(@($action.requires) + @($action.inputs.Keys) | Where-Object { -not $available.ContainsKey($_) })
                if ($blocked.Count -gt 0) { continue }
                foreach ($key in $action.outputs.Keys) {
                    if (-not $available.ContainsKey($key)) { $available[$key] = $true; $changed = $true }
                }
            }
        } while ($changed)
        $unreachable = @($Data.entities | Where-Object { $ranks[$_.status] -le $ranks[$stage] -and -not $available.ContainsKey($_.key) })
        $blockedKeys = @($unreachable | ForEach-Object { $_.key })
        Assert-That ($unreachable.Count -eq 0) "dependency: $stage unbootstrappable cycle/missing source: $($blockedKeys -join ', ')."
    }
    foreach ($source in $Data.sources) {
        Assert-That ($actions.Contains($source.action)) "reference: source $($source.action)."
        if ($null -ne $source.renewalGameHours) { Assert-PositiveInteger $source.renewalGameHours "$($source.action) renewal" }
        foreach ($value in $source.proposedYieldRange) { Assert-PositiveInteger $value "$($source.action) yield range" }
        Assert-That ($source.proposedYieldRange[0] -le $source.proposedYieldRange[1]) "quantity: inverted range $($source.action)."
    }
    $bootstrap = [ordered]@{}
    foreach ($key in $Data.bootstrap.supply.Keys) {
        Assert-That ($actions.Contains($key)) "reference: bootstrap $key."
        Assert-PositiveInteger $Data.bootstrap.supply[$key] "bootstrap $key"
        $action = $actions[$key]
        foreach ($required in $action.requires) {
            Assert-That ($Data.starting.inventory.Contains($required)) "bootstrap: source $key requires unavailable $required."
        }
        Add-Stock $bootstrap $action.outputs $Data.bootstrap.supply[$key]
    }
    Assert-That ($actions.Contains($Data.bootstrap.target)) 'reference: bootstrap target.'
    foreach ($key in $actions[$Data.bootstrap.target].inputs.Keys) {
        Assert-That ($bootstrap.Contains($key) -and $bootstrap[$key] -ge $actions[$Data.bootstrap.target].inputs[$key]) "bootstrap: insufficient $key."
    }

    $opening = $Data.openingBudget
    $supply = [ordered]@{}
    $demand = [ordered]@{}
    $sourceActions = 0
    foreach ($key in $opening.sourceAssumptions.Keys) {
        Assert-That ($actions.Contains($key)) "reference: source assumption $key."
        Assert-PositiveInteger $opening.sourceAssumptions[$key] "source $key"
        Add-Stock $supply $actions[$key].outputs $opening.sourceAssumptions[$key]
        $sourceActions += $opening.sourceAssumptions[$key]
    }
    foreach ($key in $opening.constructionActions.Keys) {
        Assert-That ($actions.Contains($key)) "reference: construction $key."
        Assert-PositiveInteger $opening.constructionActions[$key] "construction $key"
        Add-Stock $demand $actions[$key].inputs $opening.constructionActions[$key]
    }
    Assert-StockEqual $demand $opening.expectedConstructionInputs 'construction'
    foreach ($key in $opening.foodActions.Keys) {
        Assert-That ($actions.Contains($key)) "reference: food $key."
        Assert-PositiveInteger $opening.foodActions[$key] "food $key"
        Add-Stock $demand $actions[$key].inputs $opening.foodActions[$key]
    }
    foreach ($key in $demand.Keys) {
        Assert-That ($supply.Contains($key) -and $supply[$key] -ge $demand[$key]) "supply: insufficient $key for opening demand $($demand[$key])."
    }
    Assert-That ($sourceActions -eq $opening.expectedSourceActions) 'supply: action count mismatch.'
    $pack = [ordered]@{}
    Add-Stock $pack $Data.starting.inventory
    $stored = [ordered]@{}
    $conditions = @{}
    foreach ($key in $Data.starting.conditions) { $conditions[$key] = 1 }
    $peak = 0
    $observedActions = [ordered]@{}
    foreach ($step in $opening.steps) {
        if ($step.Contains('action')) {
            Assert-That ($actions.Contains($step.action)) "reference: step $($step.action)."
            Assert-PositiveInteger $step.count "step $($step.action)"
            $action = $actions[$step.action]
            if (-not $observedActions.Contains($step.action)) { $observedActions[$step.action] = 0 }
            $observedActions[$step.action] += $step.count
            for ($i = 0; $i -lt $step.count; $i++) {
                foreach ($required in $action.requires) {
                    $owned = ($pack.Contains($required) -and $pack[$required] -gt 0) -or $conditions.ContainsKey($required)
                    Assert-That $owned "prerequisite: $($step.action) needs $required now, not later."
                }
                Add-Stock $pack $action.inputs -1
                foreach ($key in $action.outputs.Keys) {
                    if ($entities[$key].capacity -eq 0) {
                        $conditions[$key] = 1
                    } else {
                        Add-Stock $pack ([ordered]@{ $key = $action.outputs[$key] })
                    }
                }
                $peak = [math]::Max($peak, (Assert-Capacity $pack $stored $entities $Data.units.inventoryCapacity))
            }
        } else {
            Assert-That ($conditions.ContainsKey('piece:Chest')) 'prerequisite: transfer before chest exists.'
            if ($step.Contains('store')) {
                Test-References $step.store $entities 'store'
                Add-Stock $pack $step.store -1
                Add-Stock $stored $step.store
            } elseif ($step.Contains('take')) {
                Test-References $step.take $entities 'take'
                Add-Stock $stored $step.take -1
                Add-Stock $pack $step.take
            } else { throw 'schema: unknown opening step.' }
            $peak = [math]::Max($peak, (Assert-Capacity $pack $stored $entities $Data.units.inventoryCapacity))
        }
    }
    foreach ($expected in @($opening.sourceAssumptions, $opening.constructionActions, $opening.foodActions)) {
        foreach ($key in $expected.Keys) {
            Assert-That ($observedActions.Contains($key) -and $observedActions[$key] -eq $expected[$key]) "budget: sequence differs for $key."
        }
    }
    Assert-That ($peak -eq $opening.expectedPeakPack) "capacity: opening peak expected $($opening.expectedPeakPack), got $peak."
    Assert-StockEqual $pack $opening.expectedFinalPack 'opening final pack'
    Assert-StockEqual $stored $opening.expectedFinalStored 'opening final chest'

    $winter = $Data.winterBudget
    Test-References $winter.reserve $entities 'winter reserve'
    Assert-That ($winter.awakeHoursPerDay + $winter.sleepHoursPerDay -eq 24) 'winter: daily hours.'
    $hunger = $winter.days * ($winter.awakeHoursPerDay * $Data.units.awakeHungerPerGameHour + $winter.sleepHoursPerDay * $Data.units.sleepHungerPerGameHour)
    Assert-That ([math]::Abs($hunger - $winter.expectedHungerDemand) -lt 0.000001) 'winter: hunger arithmetic.'
    $nutrition = $winter.reserve['item:RoastedRoots'] * $entities['item:RoastedRoots'].nutrition
    Assert-That ($nutrition -eq $winter.expectedNutrition -and $nutrition -ge $hunger) 'winter: insufficient food nutrition.'
    $fuel = $winter.reserve['item:Branch'] * $Data.units.branchFuelGameHours
    Assert-That ($fuel -eq $winter.expectedFuelHours -and $fuel -ge $winter.days * $winter.fireHoursPerDay) 'winter: insufficient fuel.'
    Assert-That ((Get-Capacity $winter.reserve $entities) -eq $winter.expectedReserveCapacity) 'winter: reserve capacity.'
    Assert-That ($winter.rootPlots * $winter.cycles -eq $winter.rootHarvests) 'winter: crop cycles.'
    Assert-That ($winter.cycles * $winter.assumedCycleGameHours -le $Data.units.seasonDays * 24) 'winter: production exceeds preparation season.'
    $roots = $winter.rootHarvests * $winter.rootYieldPerHarvest
    $seeds = $winter.rootHarvests * $winter.seedYieldPerHarvest
    Assert-That ($winter.rootYieldPerHarvest -eq $actions['garden:HarvestRoots'].outputs['item:Roots']) 'winter: root source drift.'
    Assert-That ($winter.seedYieldPerHarvest -eq $actions['garden:HarvestRoots'].outputs['item:Seeds']) 'winter: seed source drift.'
    Assert-That ($roots -ge $winter.reserve['item:RoastedRoots'] * $actions['craft:RoastedRoots'].inputs['item:Roots']) 'winter: roots insufficient.'
    Assert-That ($seeds -ge $winter.rootHarvests * $winter.seedCostPerPlanting) 'winter: seed renewal insufficient.'
    $pack = [ordered]@{}
    Add-Stock $pack $winter.stockingStart
    $stored = [ordered]@{}
    $winterPeak = 0
    foreach ($step in $winter.stockingSteps) {
        if ($step.Contains('receive')) {
            Test-References $step.receive $entities 'winter receive'
            Assert-That ($step.Contains('provenance')) 'winter: received stock needs provenance.'
            Add-Stock $pack $step.receive
        } elseif ($step.Contains('cookRoasted')) {
            Assert-PositiveInteger $step.cookRoasted 'winter cook'
            Add-Stock $pack $actions['craft:RoastedRoots'].inputs (-$step.cookRoasted)
            Add-Stock $pack $actions['craft:RoastedRoots'].outputs $step.cookRoasted
        } elseif ($step.Contains('store')) {
            Test-References $step.store $entities 'winter store'
            Add-Stock $pack $step.store -1
            Add-Stock $stored $step.store
        } else { throw 'schema: unknown winter stocking step.' }
        $winterPeak = [math]::Max($winterPeak, (Assert-Capacity $pack $stored $entities $Data.units.inventoryCapacity))
    }
    Assert-That ($winterPeak -eq $winter.expectedStockingPeakPack) "capacity: winter peak expected $($winter.expectedStockingPeakPack), got $winterPeak."
    Assert-StockEqual $pack $winter.expectedStockingFinalPack 'winter final pack'
    Assert-StockEqual $stored $winter.reserve 'winter final chest'
    return "Catalog valid: $($entities.Count) entities, $($actions.Count) actions; current/next/later dependency paths reachable in their DESIGN tiers. Opening peak $peak/120; winter peak $winterPeak/120, reserve $($winter.expectedReserveCapacity)/120; nutrition $nutrition vs $hunger. No gameplay/playtest claim."
}

function Test-SourceParity($Data) {
    $header = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\Simulation\HomesteadSimulation.h') -Raw
    $source = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\Simulation\HomesteadSimulation.cpp') -Raw
    $controller = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\HomesteadController.cpp') -Raw
    foreach ($enum in $Data.enums.Keys) {
        $match = [regex]::Match($header, "(?s)enum class $enum\s*:\s*int\s*\{(?<body>.*?)\}")
        Assert-That $match.Success "source: enum $enum missing."
        $names = @($match.Groups['body'].Value.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne 'Count' })
        Assert-That (($names -join ',') -ceq ($Data.enums[$enum] -join ',')) "source: $enum changed from baseline; review IDs, do not silently resnapshot."
    }
    $actions = Get-Index $Data.actions 'action'
    foreach ($definition in @(
        @{function='Yield'; type='ResourceKind'; prefix='gather'},
        @{function='CraftChange'; type='Recipe'; prefix='craft'},
        @{function='BuildCost'; type='Piece'; prefix='build'}
    )) {
        $body = [regex]::Match($source, "(?s)Inventory $($definition.function)\([^)]*\)\s*\{(?<body>.*?)\r?\n\}").Groups['body'].Value
        Assert-That ($body.Length -gt 0) "source: $($definition.function) missing."
        foreach ($name in $Data.enums[$definition.type]) {
            $cost = [regex]::Match($body, "case $($definition.type)::$name\s*:\s*return Items\((?<items>.*?)\);")
            Assert-That $cost.Success "source: missing $($definition.type)::$name."
            $inputs = [ordered]@{}; $outputs = [ordered]@{}
            foreach ($pair in [regex]::Matches($cost.Groups['items'].Value, 'Item::(\w+),\s*(-?\d+)')) {
                $key = "item:$($pair.Groups[1].Value)"
                $quantity = [int]$pair.Groups[2].Value
                if ($quantity -lt 0) { $inputs[$key] = -$quantity } else { $outputs[$key] = $quantity }
            }
            $action = $actions["$($definition.prefix):$name"]
            Assert-StockEqual $action.inputs $inputs "source $name inputs"
            if ($definition.prefix -ne 'build') { Assert-StockEqual $action.outputs $outputs "source $name outputs" }
        }
    }
    foreach ($entry in @(@('LinenTunic',12),@('LinenApron',6),@('WovenFootwraps',8))) {
        Assert-That ([regex]::IsMatch($source, "(?s)\{WearableDefinition::$($entry[0]),.*?, $($entry[1])\}")) "source: garment $($entry[0])."
        Assert-That ($actions["garment:$($entry[0])"].inputs['item:Fiber'] -eq $entry[1]) "source: garment cost $($entry[0])."
    }
    $entities = Get-Index $Data.entities 'entity'
    foreach ($name in @('Berries','RoastedRoots','HerbedRoots')) {
        $nutrition = [regex]::Match($source, "case Item::$name\s*:\s*return ([0-9.]+);")
        Assert-That ($nutrition.Success -and [double]$nutrition.Groups[1].Value -eq $entities["item:$name"].nutrition) "source: nutrition $name."
    }
    foreach ($entry in $Data.sources | Where-Object { $actions[$_.action].status -eq 'current' }) {
        $name = $entry.action.Split(':')[1]
        $renewal = [regex]::Match($source, "case ResourceKind::$name\s*:\s*return ([0-9.]+);")
        Assert-That ($renewal.Success -and [double]$renewal.Groups[1].Value -eq $entry.renewalGameHours) "source: renewal $name."
    }
    foreach ($action in $Data.actions | Where-Object { $_.status -eq 'current' -and $_.kind -in @('craft','build') }) {
        $hours = if ($action.kind -eq 'craft') { 0.05 } else { 0.1 }
        Assert-That ($action.nativeAdvanceGameHours -eq $hours) "source: catalog timing $($action.key)."
    }
    Assert-StockEqual $actions['clear:Sapling'].outputs $actions['gather:Sapling'].outputs 'source ready-clear yield'
    Assert-That ($Data.actionDefaults.authorityGameHours -eq 0 -and $Data.actionDefaults.directEnergyPoints -eq 0) 'source: no current per-action time/energy charge.'
    Assert-That ($controller.Contains('if (Result.ok) Sim.AdvanceGameHours(0.05, PlayerPoint());')) 'source: recipe wrapper timing changed.'
    Assert-That ($controller.Contains('if (Result.ok) Sim.AdvanceGameHours(0.1, Position);')) 'source: placement wrapper timing changed.'
    Assert-That ($header.Contains('InventoryCapacity = 120')) 'source: capacity changed.'
    Write-Output 'Source parity valid for current enum IDs, gather/recipe/build/garment quantities and wrapper timing anchors.'
}

$data = Get-Content -LiteralPath $CatalogPath -Raw | ConvertFrom-Json -AsHashtable
Test-Catalog $data
if (-not $SkipSourceParity) { Test-SourceParity $data }
if ($SelfTest) {
    $cases = @(
        @{name='unknown reference'; expected='reference:'; mutate={param($d) $d.actions[0].outputs['item:Unknown'] = 1}},
        @{name='nonpositive quantity'; expected='quantity:'; mutate={param($d) $d.actions[0].outputs['item:Branch'] = 0}},
        @{name='duplicate key'; expected='duplicate:'; mutate={param($d) $d.entities += $d.entities[0]}},
        @{name='tool dependency cycle'; expected='dependency:'; mutate={param($d) $d.actions[0].requires = @('item:Hatchet')}},
        @{name='station dependency cycle'; expected='dependency:'; mutate={param($d) ($d.actions | Where-Object key -eq 'next:Workbench').requires += 'station:Workbench'}},
        @{name='unknown station'; expected='reference:'; mutate={param($d) ($d.actions | Where-Object key -eq 'craft:Hatchet').requires += 'station:Missing'}},
        @{name='pack overflow'; expected='capacity:'; mutate={param($d) $d.winterBudget.stockingSteps[0].receive['item:Roots'] = 121}},
        @{name='chest overflow'; expected='capacity:'; mutate={param($d) $d.winterBudget.stockingSteps += @{store=@{'item:Knife'=1;'item:Hatchet'=1;'item:DiggingStick'=1;'item:WateringCan'=1}}; $d.winterBudget.stockingSteps += @{receive=@{'item:Stone'=40};provenance='negative test'}; $d.winterBudget.stockingSteps += @{store=@{'item:Stone'=40}}}},
        @{name='undersupplied neighborhood'; expected='supply:'; mutate={param($d) $d.openingBudget.sourceAssumptions['gather:Stones'] = 2}},
        @{name='early unavailable tool'; expected='prerequisite:'; mutate={param($d) $d.openingBudget.steps = @(@{action='clear:Sapling';count=1}) + $d.openingBudget.steps}},
        @{name='future runtime ID'; expected='id:'; mutate={param($d) ($d.entities | Where-Object key -eq 'item:Timber').runtimeId = 14}}
    )
    foreach ($case in $cases) {
        $copy = $data | ConvertTo-Json -Depth 50 | ConvertFrom-Json -AsHashtable
        & $case.mutate $copy
        $diagnostic = $null
        try { $null = Test-Catalog $copy } catch { $diagnostic = $_.Exception.Message }
        Assert-That ($null -ne $diagnostic -and $diagnostic.StartsWith($case.expected)) "self-test: $($case.name) failed for wrong reason or passed: $diagnostic"
        Write-Output "Expected rejection: $($case.name) [$($case.expected.TrimEnd(':'))]"
    }
}
