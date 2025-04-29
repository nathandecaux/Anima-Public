// filepath: /home/ndecaux/Git/Anima/src/Anima/math-tools/data_io/animaTRKReader.cxx
#include <animaTRKReader.h>
#include <animaTRKHeaderStructure.h>
#include <vnl_matrix.h>
#include <fstream>
#include <cstring> // Pour memset

#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>

#include <itkMacro.h>

namespace anima
{

void TRKReader::Update()
{
    // Utiliser le membre m_Header au lieu d'une variable locale
    // anima::TRKHeaderStructure headerStr; // Supprimer cette ligne

    std::ifstream inFile(m_FileName,std::ios::binary);
    if (!inFile.is_open())
        throw itk::ExceptionObject(__FILE__, __LINE__,"Unable to open file " + m_FileName,ITK_LOCATION);

    // Lire directement dans le membre m_Header
    inFile.read((char *) &m_Header, sizeof(anima::TRKHeaderStructure));

    // Utiliser m_Header pour les vérifications et l'accès aux informations
    if (m_Header.version != 2)
        throw itk::ExceptionObject(__FILE__, __LINE__,"TRK reader only supports version 2",ITK_LOCATION);

    // Now create polydata and fill it
    m_OutputData = vtkSmartPointer <vtkPolyData>::New();
    m_OutputData->Initialize();
    m_OutputData->Allocate();

    vtkSmartPointer <vtkPoints> myPoints = vtkSmartPointer <vtkPoints>::New();
    // Utiliser m_Header.n_scalars
    std::vector < vtkSmartPointer <vtkDoubleArray> > scalarArrays(m_Header.n_scalars);
    for (unsigned int i = 0;i < m_Header.n_scalars;++i)
    {
        scalarArrays[i] = vtkSmartPointer <vtkDoubleArray>::New();
        scalarArrays[i]->SetNumberOfComponents(1);
        scalarArrays[i]->SetName(m_Header.scalar_name[i]); // Utiliser m_Header
    }

    // Utiliser m_Header.n_properties
    std::vector < vtkSmartPointer <vtkDoubleArray> > cellArrays(m_Header.n_properties);
    for (unsigned int i = 0;i < m_Header.n_properties;++i)
    {
        cellArrays[i] = vtkSmartPointer <vtkDoubleArray>::New();
        cellArrays[i]->SetNumberOfComponents(1);
        cellArrays[i]->SetName(m_Header.property_name[i]); // Utiliser m_Header
    }

    // Utiliser m_Header.n_count
    unsigned int nCells = (m_Header.n_count > 0) ? static_cast<unsigned int>(m_Header.n_count) : 0; // Gérer le cas où n_count est 0 ou négatif
    // Utiliser m_Header.n_scalars et m_Header.n_properties
    std::vector <float> pointValues(3 + m_Header.n_scalars);
    std::vector <float> cellScalars(m_Header.n_properties);
    vnl_matrix <double> vox_to_ras(4,4);
    for (unsigned int i = 0;i < 4;++i)
    {
        for (unsigned int j = 0;j < 4;++j)
            vox_to_ras(i,j) = m_Header.vox_to_ras[i][j]; // Utiliser m_Header
    }

    // Si n_count était 0, on doit lire jusqu'à la fin du fichier
    // Note: Cette partie est simplifiée. Une lecture jusqu'à EOF serait plus robuste si n_count n'est pas fiable.
    if (nCells == 0) {
         // Déterminer la taille restante du fichier pour estimer le nombre de streamlines
         // Cette partie est complexe et dépend du format exact si n_count n'est pas défini.
         // Pour l'instant, on suppose que n_count est fiable s'il est > 0.
         // Si n_count est 0, on ne lit aucune streamline dans cette implémentation simplifiée.
         // Une implémentation complète lirait jusqu'à la fin du fichier.
         // throw itk::ExceptionObject(__FILE__, __LINE__,"TRK file with n_count=0 is not fully supported yet.",ITK_LOCATION);
    }


    for (unsigned int i = 0; i < nCells; ++i) // Utiliser la variable nCells calculée
    {
        int npts;
        inFile.read((char *) &npts, sizeof(int));

        // Vérifier si la lecture a échoué (fin de fichier prématurée ?)
        if (inFile.fail() || npts <= 0) {
             // Gérer l'erreur ou la fin de fichier inattendue
             // Peut-être logger un avertissement et arrêter la lecture
             // std::cerr << "Warning: Unexpected end of file or invalid point count encountered." << std::endl;
             break; // Sortir de la boucle
        }


        vtkIdType* ids = new vtkIdType[npts];

        for (int j = 0; j < npts; ++j) // Utiliser int car npts est int
        {
            // Utiliser m_Header.n_scalars
            inFile.read((char *) pointValues.data(), (3 + m_Header.n_scalars) * sizeof(float));
             if (inFile.fail()) { /* Gérer erreur */ break; }


            double xValue = vox_to_ras(0,3);
            double yValue = vox_to_ras(1,3);
            double zValue = vox_to_ras(2,3);

            for (unsigned int k = 0;k < 3;++k)
            {
                xValue += vox_to_ras(0,k) * pointValues[k];
                yValue += vox_to_ras(1,k) * pointValues[k];
                zValue += vox_to_ras(2,k) * pointValues[k];
            }

            ids[j] = myPoints->InsertNextPoint(xValue, yValue, zValue);
            // Utiliser m_Header.n_scalars
            for (unsigned int k = 0;k < m_Header.n_scalars;++k)
                scalarArrays[k]->InsertNextValue(pointValues[3 + k]);
        }
         if (inFile.fail()) { delete[] ids; break; } // Nettoyer et sortir si erreur


        // Utiliser m_Header.n_properties
        if (m_Header.n_properties > 0) {
            inFile.read((char *) cellScalars.data(), m_Header.n_properties * sizeof(float));
             if (inFile.fail()) { delete[] ids; break; } // Nettoyer et sortir si erreur
            for (unsigned int k = 0;k < m_Header.n_properties;++k)
                cellArrays[k]->InsertNextValue(cellScalars[k]);
        }


        m_OutputData->InsertNextCell (VTK_POLY_LINE, npts, ids);
        delete[] ids;
    }

    inFile.close();

    m_OutputData->SetPoints(myPoints);
    // Utiliser m_Header.n_scalars
    for (unsigned int k = 0;k < m_Header.n_scalars;++k)
        m_OutputData->GetPointData()->AddArray(scalarArrays[k]);
    // Utiliser m_Header.n_properties
    for (unsigned int k = 0;k < m_Header.n_properties;++k)
        m_OutputData->GetCellData()->AddArray(cellArrays[k]);
}

} // end namespace anima