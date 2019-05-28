#import "MetalLayer.h"
#import <JavascriptCore/JavascriptCore.h>

class HydraMetalLayer : public MetalLayer {
	
	private:
		
		JSContext *context; 
		
		id<MTLBuffer> _timeBuffer;
		id<MTLBuffer> _resolutionBuffer;
		id<MTLBuffer> _mouseBuffer;
		
		id<MTLTexture> _o0;
		id<MTLTexture> _o1;
		id<MTLTexture> _o2;
		id<MTLTexture> _o3;
		
		std::vector<id> _params;
		std::vector<NSMutableArray *> _uniform;

		std::vector<id<MTLBuffer>> _argumentEncoderBuffer;

		double _starttime;
		
		std::vector<NSString *> _uniforms;


	public:
		
		id<MTLTexture> o0() { return this->_o0;  }
		
		
		bool setup() {
			
			this->context = [JSContext new];
			
			this->_starttime = CFAbsoluteTimeGetCurrent();
			
			MTLTextureDescriptor *texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:this->_width height:this->_height mipmapped:NO];
			if(!texDesc) return false;
			
			this->_timeBuffer = [this->_device newBufferWithLength:sizeof(float) options:MTLResourceOptionCPUCacheModeDefault];
			if(!this->_timeBuffer) return false;
			
			this->_resolutionBuffer = [this->_device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault];
			if(!this->_resolutionBuffer) return false;
			
			float *resolutionBuffer = (float *)[this->_resolutionBuffer contents];
			resolutionBuffer[0] = this->_width;
			resolutionBuffer[1] = this->_height;
			
			[this->context evaluateScript:[NSString stringWithFormat:@"resolution={x:%f,y:%f};",resolutionBuffer[0],resolutionBuffer[1]]]; 
			
			this->_mouseBuffer = [this->_device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault];
			if(!this->_mouseBuffer) return false;
			
			this->_o0 = [this->_device newTextureWithDescriptor:texDesc];
			if(!this->_o0)  return false;
			
			this->_o1 = [this->_device newTextureWithDescriptor:texDesc];
			if(!this->_o1)  return false;
			
			this->_o2 = [this->_device newTextureWithDescriptor:texDesc];
			if(!this->_o2)  return false;
			
			this->_o3 = [this->_device newTextureWithDescriptor:texDesc];
			if(!this->_o3)  return false;
		
			if(MetalLayer::setup()==false) return false;
			
			for(int k=0; k<this->_library.size(); k++) {
				
				this->_argumentEncoderBuffer.push_back([this->_device newBufferWithLength:sizeof(float)*[this->_argumentEncoder[k] encodedLength] options:MTLResourceOptionCPUCacheModeDefault]);

				[this->_argumentEncoder[k] setArgumentBuffer:this->_argumentEncoderBuffer[k] offset:0];
				[this->_argumentEncoder[k] setBuffer:this->_timeBuffer offset:0 atIndex:0];
				[this->_argumentEncoder[k] setBuffer:this->_resolutionBuffer offset:0 atIndex:1];
				[this->_argumentEncoder[k] setBuffer:this->_mouseBuffer offset:0 atIndex:2];
				[this->_argumentEncoder[k] setTexture:this->_o0 atIndex:3];
				[this->_argumentEncoder[k] setTexture:this->_o1 atIndex:4];
				[this->_argumentEncoder[k] setTexture:this->_o2 atIndex:5];
				[this->_argumentEncoder[k] setTexture:this->_o3 atIndex:6];
					
				NSString *path = this->_uniforms.size()?this->_uniforms[k]:@"./default.json";
				NSString *json= [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
				NSData *jsonData = [json dataUsingEncoding:NSUnicodeStringEncoding];
				NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:jsonData options:NSJSONReadingAllowFragments error:nil];
				NSMutableArray *list = [NSMutableArray array];
				for(id key in [dict keyEnumerator]) {
					[list addObject:key];
				}
				list = (NSMutableArray *)[(NSArray *)list sortedArrayUsingComparator:^NSComparisonResult(NSString *s1,NSString *s2) {
					int n1 = [[s1 componentsSeparatedByString:@"_"][1] intValue];
					int n2 = [[s2 componentsSeparatedByString:@"_"][1] intValue];
					if(n1<n2) return (NSComparisonResult)NSOrderedAscending;
					else if(n1>n2) return (NSComparisonResult)NSOrderedDescending;
					else return (NSComparisonResult)NSOrderedSame;
				}];
					
				_uniform.push_back([NSMutableArray array]);
				
				for(int n=0; n<[list count]; n++) {
					
					if(n<=[_uniform[k] count]) {
						[_uniform[k] addObject:dict[list[n]]];
					}
					else {
						_uniform[k][n] = dict[list[n]];
					}
					
					if(n<=_params.size()) {
						_params.push_back((id)[_device newBufferWithLength:sizeof(float) options:MTLResourceOptionCPUCacheModeDefault]);
						[_argumentEncoder[k] setBuffer:(id<MTLBuffer>)_params[n] offset:0 atIndex:7+n];
					}
					
				}
			}
						
			return true;
		} 
		
		      
		bool init(int width,int height,std::vector<NSString *> shaders={@"defalt.metallib"},std::vector<NSString *> uniforms={@"defalt.metallib"}, bool isGetBytes=false) {
			for(int k=0; k<uniforms.size(); k++) this->_uniforms.push_back(uniforms[k]);
			return MetalLayer::init(width,height,shaders,isGetBytes);
		}
			
		id<MTLCommandBuffer> setupCommandBuffer(int mode) {
						
			id<MTLCommandBuffer> commandBuffer = [this->_commandQueue commandBuffer];
			
			float *timeBuffer = (float *)[this->_timeBuffer contents];
			timeBuffer[0] = CFAbsoluteTimeGetCurrent()-this->_starttime;
			
			float *mouseBuffer = (float *)[this->_mouseBuffer contents];
			
			double x = _frame.origin.x;
			double y = _frame.origin.y;
			double w = _frame.size.width;
			double h = _frame.size.height;
			
			NSPoint mouseLoc = [NSEvent mouseLocation];
			mouseBuffer[0] = (mouseLoc.x-x);
			mouseBuffer[1] = (mouseLoc.y-y);
						
			[context evaluateScript:[NSString stringWithFormat:@"time=%f;",timeBuffer[0]]]; 
			[context evaluateScript:[NSString stringWithFormat:@"mouse={x:%f,y:%f};",mouseBuffer[0],mouseBuffer[1]]]; 
			
			for(int k=0; k< [this->_uniform[mode] count]; k++) {
				float *tmp = (float *)[(id<MTLBuffer>)this->_params[k] contents];
				if([[_uniform[mode][k] className] compare:@"__NSCFString"]==NSOrderedSame) {
					tmp[0] = [[context evaluateScript:[NSString stringWithFormat:@"(%@)();",this->_uniform[mode][k]]] toDouble];
				}
				else {
					tmp[0] = [_uniform[mode][k] doubleValue];
				}
			}
			
			MTLRenderPassColorAttachmentDescriptor *colorAttachment = this->_renderPassDescriptor.colorAttachments[0];
			colorAttachment.texture = this->_metalDrawable.texture;
			colorAttachment.loadAction  = MTLLoadActionClear;
			colorAttachment.clearColor  = MTLClearColorMake(0.0f,0.0f,0.0f,0.0f);
			colorAttachment.storeAction = MTLStoreActionStore;
			
			id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:this->_renderPassDescriptor];
			[renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
			[renderEncoder setRenderPipelineState:this->_renderPipelineState[mode]];
			[renderEncoder setVertexBuffer:this->_verticesBuffer offset:0 atIndex:0];
			//[renderEncoder setVertexBuffer:this->_texcoordBuffer offset:0 atIndex:1];
			
			[renderEncoder useResource:_timeBuffer usage:MTLResourceUsageRead];
			[renderEncoder useResource:_resolutionBuffer usage:MTLResourceUsageRead];
			[renderEncoder useResource:_mouseBuffer usage:MTLResourceUsageRead];
				
			[renderEncoder useResource:_o0 usage:MTLResourceUsageSample];
			[renderEncoder useResource:_o1 usage:MTLResourceUsageSample];
			[renderEncoder useResource:_o2 usage:MTLResourceUsageSample];
			[renderEncoder useResource:_o3 usage:MTLResourceUsageSample];
				
			for(int n=0; n<[this->_uniform[mode] count]; n++) {
				[renderEncoder useResource:(id<MTLBuffer>)this->_params[n] usage:MTLResourceUsageRead];
			}
				
			[renderEncoder setFragmentBuffer:this->_argumentEncoderBuffer[mode] offset:0 atIndex:0];
			
			[renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:Plane::INDICES_SIZE indexType:MTLIndexTypeUInt16 indexBuffer:this->_indicesBuffer indexBufferOffset:0];
			
			[renderEncoder endEncoding];
			[commandBuffer presentDrawable:this->_metalDrawable];
			this->_drawabletexture = this->_metalDrawable.texture;
			return commandBuffer;
		}
		
		HydraMetalLayer() {
			NSLog(@"HydraMetalLayer");
		}
		
		~HydraMetalLayer() {
			
		}
};