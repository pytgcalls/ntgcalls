@out @{config.self_dir}/index.mjs
import binding from './index.js';

export default binding;
@for c in classes
export const @{c.name} = binding.@{c.name};
@end
@for e in enums
@if e.emit
export const @{e.name} = binding.@{e.name};
@end
@end
export const LogLevel = binding.LogLevel;
export const setLogLevel = binding.setLogLevel;
