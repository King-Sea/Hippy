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

#import "HippyComponentMap.h"

@interface HippyComponentMap () {
    NSMutableDictionary<NSNumber *, NSMutableDictionary<NSNumber *, id<HippyComponent>> *> *_componentsMap;
}

@end

@implementation HippyComponentMap

- (instancetype)init {
    self = [super init];
    if (self) {
        _componentsMap = [NSMutableDictionary dictionaryWithCapacity:256];
    }
    return self;
}

- (void)addRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (tag && ![_componentsMap objectForKey:tag]) {
        NSMutableDictionary *dic = [NSMutableDictionary dictionaryWithCapacity:256];
        [_componentsMap setObject:dic forKey:tag];
    }
}

- (void)removeRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    [_componentsMap removeObjectForKey:tag];
}

- (void)addComponent:(__kindof id<HippyComponent>)component forRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"component and tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (component && tag) {
        id map = [_componentsMap objectForKey:tag];
        [map setObject:component forKey:[component hippyTag]];
    }
}

- (void)removeComponent:(__kindof id<HippyComponent>)component forRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"component and tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (component && tag) {
        id map = [_componentsMap objectForKey:tag];
        [map removeObjectForKey:[component hippyTag]];
    }
}

- (NSArray<__kindof id<HippyComponent>> *)componentsForRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (tag) {
        id map = [_componentsMap objectForKey:tag];
        return [map allValues];
    }
    return nil;
}

- (__kindof id<HippyComponent>)componentForTag:(NSNumber *)componentTag onRootViewTag:(NSNumber *)tag {
    NSAssert(tag, @"componentTag && tag must not be null in method %@", NSStringFromSelector(_cmd));
    if (componentTag && tag) {
        id map = [_componentsMap objectForKey:tag];
        return [map objectForKey:componentTag];
    }
    return nil;
}

@end
