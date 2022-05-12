/*!
 * iOS SDK
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>
#import "HippyComponent.h"

NS_ASSUME_NONNULL_BEGIN

@interface HippyComponentMap : NSObject

- (void)addRootViewTag:(NSNumber *)tag;

- (void)removeRootViewTag:(NSNumber *)tag;

- (void)addComponent:(__kindof id<HippyComponent>)component forRootViewTag:(NSNumber *)tag;

- (void)removeComponent:(__kindof id<HippyComponent>)component forRootViewTag:(NSNumber *)tag;

- (NSArray<__kindof id<HippyComponent>> *)componentsForRootViewTag:(NSNumber *)tag;

- (__kindof id<HippyComponent>)componentForTag:(NSNumber *)componentTag onRootViewTag:(NSNumber *)tag;

@end

NS_ASSUME_NONNULL_END
