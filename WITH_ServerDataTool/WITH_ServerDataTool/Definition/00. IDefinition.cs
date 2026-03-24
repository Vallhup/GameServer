using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WITH_ServerDataTool.Definition
{
    public interface IDefinition
    {
    }

    public interface IDefinition<TId> : IDefinition
    {
        TId Id { get; set; }
    }
}
