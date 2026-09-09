# ProcCity
This is the current code architecture, 2026/9/9. The architecture will be changed soon for extending to city level 
and general spline (or polyline) / polygon based generation.

## Public

### Block
#### Generator
BlockGeneratorActor.h， 功能：
1.调用Json block layout importor生成block layout，包括lot/reserve/building footprint和prop的空间描述 
2.调用FBlockEnvironmentPlanResolver生成Resolved Environment Plan, 生成并使用EnvironmentActor，消费Environment Plan生成ground/road/sidewalk/prop
3.调用BuildingGeneratorActor,组装building modules，生成建筑物（这部分逻辑管线还没有完全和Environment部分统一）
4.生成Block的debug可视化（目前暂缺）
#### Importer
1.BlockLayoutJsonImporter.h，功能：
2.定义Json block layout importor
#### Types
BlockLayoutEnums.h、BlockLayoutTypes.h，功能：
1.定义描述block layout的相关types，比如FBlockDefinition、FLotDefinition等

### Building
#### Generator
BuildingGeneratorActor.h，功能：
将block layout中的foot print/bound映射为building plan及其子类，并调用UBuildingRuleSet组装建筑物模块，生成建筑物
#### DataAssets
BuildingModuleSetDataAsset.h，功能:
定义UBuildingMaterialStyleDataAsset，每个DA实例对应一套建筑模块(比如roof、wall、wallwindow等)风格
BuildingMaterialStyleDataAsset.h，功能：
定义UBuildingMaterialStyleDataAsset，每个DA实例对应一套材质实例（比如roof用哪个材质、wall用哪个）
BuildingArchetypeDataAsset.h，功能：
定义 UBuildingArchetypeDataAsset，每个DA实例对应一种建筑物风格，包括模块风格、材质风格、组装方式（选择UBuildingRuleSet哪个子类）
#### GenerationRule
BuildingRuleSet.h等，定义UBuildingRuleSet及其子类，包含建筑物组装逻辑
#### Types
BuildingEnums.h, BuildingGenerationTypes.h, BuildingModuleTypes.h, BuildingPlanTypes.h， BuildingRuleTypes.h，文件名基本描述了其主要功能

### Environment
#### DataAssets
BlockEnvironmentStyleDataAsset.h，功能：
定义UBlockEnvironmentStyleDataAsset，每个实例描述一套Environment风格，包括道路、各种surface、prop模块的可选Array
#### Generator
BlockEnvironmentActor.h，功能：
消费Resolved Environment Plan，创建HISM，完成最终block environment 生成
#### Resolver
BlockEnvironmentPlanResolver.h, g功能：
定义FBlockEnvironmentPlanResolver，将block layout映射为Resolved Environment Plan
#### Types
EBlockEnvironmentElementType.h，各种env相关的enum，如Ground role、path role等
BlockEnvironmentFeatureTypes.h，定义surface/path/prop feature等，feature是block layout中path、ground等元素的类型、空间语义描述
BlockEnvironmentModuleTypes.h，定义插入BlockEnvironmentStyleDataAsset实例的surface/path/prop 的module，包括mesh、material entry以及来自feature的部分类型、空间语义
ResolvedBlockEnvironmentPlan.h，定义FResolvedBlockEnvironmentInstance，描述每个环境元素的最终描述，主要包括mesh、MaterialPayload、transform等，可以直接用于生成HISM
FResolvedBlockEnvironmentPlan.h，包括ResolvedBlockEnvironmentPlan array，是FBlockEnvironmentPlanResolver最终交付的产品

### Material
MaterialPayload/Entry相关的resolver、types、atlas mappingDA等，略。

## Private
.cpp源文件


